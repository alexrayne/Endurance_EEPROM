/*
 * EELS.c
 *
 * Created: 7/24/2019 11:58:42 AM
 *  Author: hasan.jaafar
 */
#include <EELS_Conf.h>
#include "EELS.h"



/* ==================================================================== */

#if 1
#include <assert.h>
#define ASSERT(...) assert(__VA_ARGS__)
#else
#define ASSERT(...)
#endif


// this assert sets at failures check, that leads to error result.
#if 0
// prevent error result, rising assert
#define EELS_ASSERT(...) ASSERT(__VA_ARGS__)
#else
// pass error result
#define EELS_ASSERT(...)
#endif

/* ==================================================================== */

EELSlot_t EELslot_arr[EELS_APP_SLOTS];


/* ==================================================================== */
///         proto-treads API dummy
#ifndef EELS_WAIT

#if defined(PT_THREAD)
// contiki  proto-threads api

#define EELS_PT         struct pt* pt = &(this->pt);

//  @args pt
#define EELS_RESET()    PT_INIT( &(this->pt) )
#define EELS_BEGIN()    PT_BEGIN(pt)
#define EELS_END()      PT_END(pt)
#ifndef PT_RESULT
#define EELS_RESULT(x)    do {                  \
                            PT_INIT(pt);        \
                            return x;           \
                          } while(0)
#else
#define EELS_RESULT(x)  PT_RESULT( pt, x)
#endif
#define EELS_READY()    PT_IS_NULL(pt)

//  @args pt, ok
#define EELS_WAIT( ok, statemt ) PT_WAIT_WHILE(pt, EELS_SCHEDULE( (ok = (statemt), ok) ) )

#else

// no proto-threads
#define EELS_PT
#define EELS_BEGIN()
#define EELS_END()
#define EELS_RESET()
#define EELS_READY()    true
#define EELS_RESULT(x)  return x

//  @args ok
#define EELS_WAIT( ok, statemt ) ok = statemt

#endif

#endif

#ifndef EELS_EEPROM_WAIT
// if status waits not need
#define EELS_EEPROM_WAIT()  EELS_ERROR_OK
#endif

#ifndef EELS_RELEASE
#define EELS_RELEASE(slotid)  // here need notify other threads about slot released from work
#endif

/// @brief характер операции чтения - блокирующая/неблокирующая
#ifndef EELS_EEPROM_READ_NOBLOKING
#define EELS_EEPROM_READ_NOBLOKING  0
#endif

#ifndef EELS_EEPROM_WRITE_NOBLOKING
#define EELS_EEPROM_WRITE_NOBLOKING  0
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) (x)
#endif

/* ==================================================================== */

/// EELS_Conf can override cpu specific CLZ operation as eels_clz(x)
#ifndef eels_clz
#ifdef clz
#define eels_clz(x)      clz(x)

#elif defined __CLZ
#define eels_clz(x)              __CLZ(x)

#elif defined (__GNUC__)
#define eels_clz(x)              __builtin_clz(x)

#elif defined (_MSC_VER)
#include <intrin.h>

static inline
unsigned eels_clz(unsigned x) {
    if (x == 0)
        return 32;

    unsigned long leading_zero = 0;

    if ( _BitScanReverse(&leading_zero, x) )
        return 31 - leading_zero;
    else
        // Same remarks as above
        return 32;
}

#else
#define eels_clz(x)              __clz(x)
#endif
#endif  //eels_clz

#ifndef LOG2_CEIL
#define EELS_LOG2_1(n) (((n) >= 2u) ? 1u : 0u)
#define EELS_LOG2_2(n) (((n) >= 1u<<2) ? (2 + EELS_LOG2_1((n)>>2)) : EELS_LOG2_1(n))
#define EELS_LOG2_4(n) (((n) >= 1u<<4) ? (4 + EELS_LOG2_2((n)>>4)) : EELS_LOG2_2(n))
#define EELS_LOG2_8(n) (((n) >= 1u<<8) ? (8 + EELS_LOG2_4((n)>>8)) : EELS_LOG2_4(n))
#define EELS_LOG2_CEIL(n)   (((n) >= 1u<<16) ? (16 + EELS_LOG2_8((n)>>16)) : EELS_LOG2_8(n))
#else
#define EELS_LOG2_CEIL(n)   LOG2_CEIL(n)
#endif

static
int eels_log2ceil(unsigned x){
    if (x == 0) return -1;

    enum { INTBITS = sizeof(unsigned)*8, };
    unsigned pow2 = INTBITS - eels_clz(x);
    if ( ((x-1)&x ) == 0)
        --pow2;
    return pow2;
}

EELSEpoch _EELS_ReadCounter(EELSlot_t*  this, uint32_t addr);
static EELSAddr    eels_pos_page(EELSlot_t*  this, EELSPosition x);



#ifndef EELS_PAGE_SIZE
static  EELSlotSize __page_size(EELSlot_t*  this) {return this->page_size;}
#else
#define __page_size(this)    EELS_PAGE_SIZE
#endif
/* ==================================================================== */
/* 						PUBLIC FUNCTIONS 								*/
/* ==================================================================== */


void EELS_Init(){
    for (unsigned h = 0; h < EELS_APP_SLOTS; ++h) {
        EELSlot_t*  this = EELslot_arr + h;
        this->length            = 0;
        this->_counter_max      = 0;
        this->write_index   = 0;
        this->slot_log_length   = 0;
#ifndef EELS_PAGE_SIZE
        this->page_size         = 0;
#endif
        this->page_offs         = 0;
        this->page_limit        = 0;
        EELS_RESET();
    }
}

enum {
    EELS_RECOFFS_EPOCH  = 0,
    EELS_RECSIZE_EPOCH  = 1,

    EELS_RECOFFS_CRC    = EELS_RECOFFS_EPOCH + EELS_RECSIZE_EPOCH,
    //EELS_RECSIZE_HEAD   = EELS_RECOFFS_CRC +_EELS_CRC_BYTE_LENGTH,

    EELS_RECOFFS_DATA   = EELS_RECSIZE_HEAD,

    EELS_EPOCH_NONE     = 0x80u,
    EELS_EPOCH_MAX      = EELS_EPOCH_NONE-1,

    // this epoch use for fresh log - when log no cycled over top-limit
    EELS_EPOCH0         = 0,

    EELS_EPOCH1         = 0,    // this epoch after epoch-count overloads
};

/// @brief check that slot have inited, and can contain some records
EELSError EELS_SlotOnline(EELSh slotNumber){
    if (slotNumber >= EELS_APP_SLOTS)
        return EELS_ERROR_NOSLOT; //out of boundary!

    EELSlot_t*  this = EELSlot(slotNumber);
    if (  (this->begining <= 0)
       || (this->length <= 0)
       || (this->raw_data_size <= 0)
       || (this->slot_log_length <= 0)
       )
       return EELS_ERROR_NOSLOT;

    return EELS_ERROR_OK;
}

EELSError EELS_SetSlot(EELSh slotNumber, EELSAddr begin_addr, EELSlotSize length, EELSDataLen data_length) {
	if (slotNumber >= EELS_APP_SLOTS)
		return EELS_ERROR_NOSLOT; //out of boundary!

	EELSlot_t*  this = EELSlot(slotNumber);

	EELS_RESET();

	/*user data length*/
	this->raw_data_size = data_length;
    /*Slot datalength*/
    this->slot_log_length = data_length + EELS_RECOFFS_DATA;

    unsigned page_size = __page_size(this);
#ifndef EELS_PAGE_SIZE
    unsigned page_2pwr = this->page_2pwr;
#else
    enum {
        page_2pwr = EELS_LOG2_CEIL(EELS_PAGE_SIZE),
    };
#endif

    if ( page_size > 0 ) {
        // page aligned

        // align slot bounds to pages
        this->begining      = eels_pos_page(this, begin_addr);

        unsigned ofs = begin_addr - this->begining;
        this->length        = length + ofs;

        if (ofs > this->page_offs){
            this->begining  += page_size;
            this->length    -= page_size;
        }

        // section aligned
        unsigned sec_size = page_size;
        if (this->page_limit > this->page_offs){
            sec_size = this->page_limit - this->page_offs;
        }

        unsigned page_records = 1;
        if ( sec_size >= (this->slot_log_length * 2u) ) {
            page_records = sec_size / this->slot_log_length;
        }
        else if (sec_size < this->slot_log_length)
            // TODO: provide support records > pagesize
            return  EELS_ERROR_SMALLPAGE;

        // align actual page_limit to records. eels_pred_pos need it
        this->page_limit = this->page_offs + page_records* this->slot_log_length;

        unsigned pages = (this->length >> page_2pwr);

        // last page section maybe used, need correct for it
        ofs = this->length - (pages << page_2pwr);
        if (ofs >= this->page_limit)
            ++pages;

        this->length        =  pages * page_size;
        this->_counter_max  =  pages * page_records;
        this->page_records = page_records;
    }
    else {
        this->begining      = begin_addr;
        this->length        = length;

        this->_counter_max = (length / this->slot_log_length); //readjust max counter
        this->page_records = 0;
    }

	_EELS_FindLastPos(slotNumber); //now
	//printf("Max counter: %d", _slot_arr[slotNumber]._counter_max);
	//printf(" - counter bytes: %d\r\n", _slot_arr[slotNumber]._counter_bytes);

	return EELS_ERROR_OK;
}

EELSError EELS_PageAlign  (EELSh slotNumber, EELSPageLen page_size){
    if (slotNumber >= EELS_APP_SLOTS)
        return EELS_ERROR_NOSLOT; //out of boundary!

    EELSlot_t*  this = EELSlot(slotNumber);

#ifndef EELS_PAGE_SIZE
    this->page_size     = page_size;
    this->page_2pwr     = eels_log2ceil(page_size);
#endif
    this->page_offs     = 0;
    this->page_limit    = (EELSPageLen)page_size;

    return EELS_ERROR_OK;
}

EELSError EELS_PageSection(EELSh slotNumber, EELSPageLen page_offs, EELSPageLen sec_size){

    if (slotNumber >= EELS_APP_SLOTS)
        return EELS_ERROR_NOSLOT; //out of boundary!

    EELSlot_t*  this = EELSlot(slotNumber);

    if ( (page_offs + sec_size) > __page_size(this))
        return  EELS_ERROR_SMALLPAGE;

    this->page_offs     = page_offs;
    this->page_limit    = page_offs + sec_size;

    return EELS_ERROR_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////
///         Position arithmetics

static
EELSAddr    eels_pos_ok(EELSlot_t*  this, EELSPosition x){
    return (x >= this->begining) && (x < (this->begining+ this->length) );
}


static
EELSAddr    eels_pos_page(EELSlot_t*  this, EELSPosition x){
#ifdef EELS_PAGE_SIZE
    EELSAddr page_mask = EELS_PAGE_SIZE-1;
#else
    EELSAddr page_mask = this->page_size -1;
#endif

    return x & ~page_mask;
}

static
EELSAddr    eels_pos_ofs(EELSlot_t*  this, EELSPosition x){
#ifdef EELS_PAGE_SIZE
    EELSAddr page_mask = EELS_PAGE_SIZE-1;
#else
    EELSAddr page_mask = this->page_size -1;
#endif

    return x & page_mask;
}

static
EELSPosition eels_pos(EELSlot_t*  this, EELSAddr page, EELSAddr ofs){
    (void)this;
    return page + ofs;
}

static
unsigned    eels_page_idx(EELSlot_t*  this, EELSAddr page){
#ifdef EELS_PAGE_SIZE
    enum { page_2pwr = EELS_LOG2_CEIL(EELS_PAGE_SIZE) , };
    return this->page_records *  ( (page - this->begining) >> page_2pwr );
#else
    return this->page_records *  ( (page - this->begining) >> this->page_2pwr );
#endif
}


static
EELSAddr    eels_index_pos(EELSlot_t*  this, unsigned idx){

    if (this->page_records == 0){
        return this->begining + (this->slot_log_length * idx);
    }
    else {
        unsigned pages = idx / this->page_records;
        EELSAddr page = this->begining + __page_size(this) * pages;

        EELSAddr ofs  = this->page_offs;
        if (this->page_records > 1){
            unsigned  pidx  = idx - (pages * this->page_records);
            ofs  += (this->slot_log_length * pidx);
        }

        return eels_pos(this, page, ofs);
    }
}

static bool eels_overlaped(EELSlot_t*  this) {return this->overlaped;}

static
EELSAddr    eels_relative_pos(EELSlot_t*  this, int idx){
    if ( eels_overlaped(this) ) {
        // log is cycled over top

        if ( idx >= this->_counter_max ) {
            return -1;  //tail
        }
        else if ( -idx >= this->_counter_max ) {
            return 0;   //head
        }

        unsigned now_counter = this->write_index;
        idx += now_counter;
        if (idx >= this->_counter_max)
            idx -= this->_counter_max;
        else if (idx < 0)
            idx += this->_counter_max;

    }
    else {
        // fresh log have records [0 ... write_index]
        int now_counter = this->write_index;

        if ( idx >= now_counter ) {
            return -1;  //tail
        }
        else if ( -idx >= now_counter ) {
            return 0;   //head
        }

        if ( idx < 0 )
            idx += now_counter;
    }
    return  eels_index_pos(this, idx);
}

static
EELSPosition    eels_next_pos(EELSlot_t*  this, EELSPosition x){
    if (__page_size(this) <= 0) {
        x += this->slot_log_length;
        if (x >= (this->begining + this->length) )
            x = this->begining;
        return x;
    }

    EELSAddr page   = eels_pos_page(this, x);
    EELSAddr ofs    = eels_pos_ofs(this, x);

    EELSlotSize page_limit;
    if (this->page_limit > 0)
        page_limit = this->page_limit;
    else
        page_limit = __page_size(this);

    ofs += this->slot_log_length;
    if ( (ofs + this->slot_log_length) > page_limit){
        ofs = this->page_offs;
        page += __page_size(this);

        if ( (page + __page_size(this)) > (this->begining + this->length) )
            page = this->begining;
    }
    return eels_pos(this, page, ofs);
}

static
EELSPosition    eels_pred_pos(EELSlot_t*  this, EELSPosition x){
    if (__page_size(this) <= 0) {
        if (x < (this->begining + this->slot_log_length) ){
            x = this->begining;
            x += this->slot_log_length * this->_counter_max; //(this->length);
        }

        x -= this->slot_log_length;
        return x;
    }

    EELSAddr page   = eels_pos_page(this, x);
    EELSAddr ofs    = eels_pos_ofs(this, x);

    if ( ofs > (unsigned)(this->page_offs + this->slot_log_length) ) {
        ofs -= this->slot_log_length;
    }
    else {

        EELSlotSize page_limit;
        if (this->page_limit > 0)
            page_limit = this->page_limit;
        else
            page_limit = __page_size(this);

        ofs = page_limit - this->slot_log_length;

        if (page < (this->begining + __page_size(this)) ) {
            page = (this->begining + this->length);
        }

        page -= __page_size(this);
    }
    return eels_pos(this, page, ofs);
}



///////////////////////////////////////////////////////////////////////////////////////////

#ifndef EELS_Ready

bool EELS_Ready(EELSh slotNumber)
{
    EELSlot_t* this = EELSlot(slotNumber);
    EELS_PT

    return  EELS_READY();
}

#endif



static
EELSEpoch   next_epoch(EELSEpoch x){
    return  (x + 1) & ~EELS_EPOCH_NONE;

    //if (epoch >= EELS_EPOCH_MAX)
    //    epoch = EELS_EPOCH1;
}

static
bool        is_next_epoch(EELSEpoch now, EELSEpoch next){
    if ( ( (now | next) & EELS_EPOCH_NONE) != 0)
        return false;

    return   next == next_epoch(now);
}


static
void eels_write_advance(EELSlot_t*  this){
    EELSPosition mypos= this->write_position;

    this->write_position = eels_next_pos(this, this->write_position);

    EELSlotLen cnt = this->write_index;
    if (mypos > this->write_position)
        cnt = 0;
    else
        ++cnt;

    if (cnt >= this->_counter_max) {
        //if counter goes beyond counter max -> set back to 1
        cnt = 0;
        mypos= eels_index_pos(this, 0);
        this->write_position = mypos;
    }

    if (cnt == 0){
        this->overlaped      = true;
        this->epoch_counter = next_epoch(this->epoch_counter);
    }

    this->write_index = cnt;
}

EELSError __EELS_InsertLog(EELSlot_t*  this, const void* src, EELSRecHead* head) {
    int ok;
    EELS_PT;

    EELS_BEGIN();

#if (__EELS_DBG__)
    _EELS_FindLastPos(slotNumber);
#endif

    EELS_WAIT( ok, EELS_EEPROM_WAIT() );
    if (ok < 0)
        EELS_RESULT(ok);

    {
    EELSlotLen cnt    = this->write_index;
    if (cnt >= this->_counter_max)
    //if (mypos > this->write_position)
    {
        // position wraped over slot length
        cnt = 0;
        this->write_index = 0;
        this->write_position = eels_index_pos(this, 0);
    }

    }

    EELS_ASSERT( eels_pos_ok(this, this->write_position) );
    if ( !eels_pos_ok(this, this->write_position) )
        EELS_RESULT(EELS_ERROR_OUTOFSLOT);

    /*=== LETS DO A WRITE ===*/
    if (head == NULL)
        head = &(this->head);

    head->raw[EELS_RECOFFS_EPOCH]   = this->epoch_counter;

	if (head == src){
	    head->raw[EELS_RECOFFS_CRC]     = EELS_CRC8( (head+1) , this->raw_data_size);

	    // provided contigous record for write
	    EELS_WAIT( ok, EELS_EEPROM_WRITE( this->write_position, src, this->slot_log_length) );
	    if (ok < 0)
	        EELS_RESULT(ok);
	}
	else {
	    head->raw[EELS_RECOFFS_CRC]     = EELS_CRC8(src, this->raw_data_size);

	    EELS_WAIT( ok, EELS_EEPROM_WRITE( this->write_position + EELS_RECOFFS_DATA , src, this->raw_data_size) );
        if (ok < 0)
            EELS_RESULT(ok);

#if EELS_EEPROM_WRITE_NOBLOKING
        EELS_WAIT( ok, EELS_EEPROM_WAIT() );
        if (ok < 0)
            EELS_RESULT(ok);
#endif

        // complete record, by write it`s head
        EELS_WAIT( ok, EELS_EEPROM_WRITE( this->write_position, head, EELS_RECSIZE_HEAD ) );
        if (ok < 0)
            EELS_RESULT(ok);

	}

#if EELS_EEPROM_WRITE_NOBLOKING
    EELS_WAIT( ok, EELS_EEPROM_WAIT() );
    if (ok < 0)
        EELS_RESULT(ok);
#endif

	#if (__EELS_DBG__)
	    _EELS_FindLastPos(slotNumber);
	#else
		eels_write_advance(this);
	#endif

    EELS_RESULT(ok);
	EELS_END();
}

static
EELSError _EELS_InsertLog(EELSh slotNumber, const void* src, EELSRecHead* head){
    EELSlot_t*  this = EELSlot(slotNumber);

    bool nolocked = EELS_Ready(slotNumber);

    EELSError ok = __EELS_InsertLog(this, src, head);

    if ( !nolocked && EELS_Ready(slotNumber) ) // maybe should release always?
        // slot was busy and have finished op.
        EELS_RELEASE(slotNumber);

    return ok;
}



EELSError EELS_InsertRec  (EELSh slotNumber, EELSRecHead* rec){
    return _EELS_InsertLog(slotNumber, rec, rec);
}

EELSIndex EELS_InsertLog(EELSh slotNumber, const void* src){
    return _EELS_InsertLog(slotNumber, src, NULL);
}



//===================================================================================================================

EELSError EELS_CleanLog  (EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

    int ok;
    EELSPosition pos0;
    EELSEpoch epoch;

    EELS_PT;

    EELS_BEGIN();

    EELS_WAIT( ok, EELS_EEPROM_WAIT() );
    if (ok < 0)
        EELS_RESULT(ok);

    pos0 = eels_index_pos(this, 0);

    epoch = _EELS_ReadCounter(this, pos0);   //current counter and position is SLOT_BEGIN
    if ( (epoch & EELS_EPOCH_NONE) != 0 ){
        this->write_position    = pos0;
        this->write_index   = 0;
        this->overlaped         = false;
        this->epoch_counter     = next_epoch(epoch);
        EELS_RESULT(EELS_ERROR_OK);
    }

    if ( this->write_position == pos0 ){
        // already writes to [0]
        epoch = this->epoch_counter;
        this->epoch_counter = next_epoch(this->epoch_counter);
    }
    else {
        this->write_position    = pos0;
        this->write_index   = 0;
        this->epoch_counter     = next_epoch(epoch);
    }
    this->overlaped         = false;

    // break 1st log record as empty
    epoch = epoch | EELS_EPOCH_NONE;
    this->head.raw[0] = epoch;
    this->head.raw[1] = 0xff;

    // complete record, by write it`s head
    EELS_WAIT( ok, EELS_EEPROM_WRITE( this->write_position, &this->head, EELS_RECSIZE_HEAD ) );

#if EELS_EEPROM_WRITE_NOBLOKING
    EELS_WAIT( ok, EELS_EEPROM_WAIT() );
    if (ok < 0)
        EELS_RESULT(ok);
#endif

    EELS_RESULT(ok);

    EELS_END();
}



//===================================================================================================================
EELSError EELS_ReadLast(EELSh slotNumber, void* const buf){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	#if _EELS_CRC_BYTE_LENGTH == 1
		return _EELS_ReadLog(slotNumber, eels_pred_pos(this, this->write_position), buf);
	#endif

}


unsigned  EELS_Len    (EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    if ( eels_overlaped(this) )
        return this->_counter_max;
    else
        return this->write_index;
}

EELSPosition    EELS_PosIdx (EELSh slotNumber, int idx){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    return eels_relative_pos(this, idx);
}


EELSPosition    EELS_PosNext(EELSh slotNumber, EELSPosition from){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    return eels_next_pos(this, from);
}

EELSError EELS_ReadPos(EELSh slotNumber, EELSPosition at , void* const buf ){
    return _EELS_ReadLog(slotNumber, at, buf);
}

EELSError EELS_ReadIdx(EELSh slotNumber, unsigned rec_idx , void* const buf ){
    return _EELS_ReadLog(slotNumber, EELS_PosIdx(slotNumber, rec_idx), buf);
}



EELSError    EELS_ReadFromHead    (EELSh slotNumber, int log_num , void* const dst){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    int32_t log_num_pos = 0;

    #if (__EELS_DBG__)
    _EELS_FindLastPos(slotNumber);
    #endif

    log_num_pos = eels_relative_pos(this, log_num);

    if (log_num_pos < 0 ) //(this->begining)
        return _EELS_ReadLog(slotNumber, log_num_pos, dst);
    else
        return EELS_ERROR_NOSLOT;
}



/* ==================================================================== */
/* 						PRIVATE FUNCTIONS 								*/
/* ==================================================================== */

uint8_t EELS_crc8(const void *src, EELSDataLen len)
{
    const uint8_t* data = (const uint8_t*)src;

	uint8_t crc = 0xff;
	uint8_t i, j;
	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if ((crc & 0x80) != 0)
			crc = (uint8_t)((crc << 1) ^ 0x31);
			else
			crc <<= 1;
		}
	}
	return crc;
}


EELSError __EELS_ReadLog(EELSlot_t*  this, EELSAddr log_start_position, void* const dst){
    EELS_PT;
    uint8_t* buf = (uint8_t*)dst;
    EELSError ok;

    EELS_BEGIN();

    EELS_ASSERT( eels_pos_ok(this, log_start_position) );
    if ( !eels_pos_ok(this, log_start_position) )
        EELS_RESULT(EELS_ERROR_OUTOFSLOT);


    EELS_WAIT( ok, EELS_EEPROM_WAIT() );
    if ( UNLIKELY(ok < 0) )
        EELS_RESULT(ok);

    EELS_WAIT( ok, EELS_EEPROM_READ(log_start_position + EELS_RECOFFS_DATA ,
							buf, this->raw_data_size ) );
    if ( UNLIKELY(ok < 0) )
        EELS_RESULT(ok);

#if EELS_EEPROM_READ_NOBLOKING
    EELS_WAIT( ok, EELS_EEPROM_WAIT() );
    if (ok < 0)
        EELS_RESULT(ok);
#endif

	{
	    uint8_t* slotcrc = this->head.raw ;
	    ok = EELS_EEPROM_READ(log_start_position + EELS_RECOFFS_CRC,
	                            slotcrc, _EELS_CRC_BYTE_LENGTH );
#if EELS_EEPROM_READ_NOBLOKING
        EELS_WAIT( ok, EELS_EEPROM_WAIT() );
        if (ok < 0)
            EELS_RESULT(ok);
#endif
        if ( UNLIKELY(ok < 0) )
            EELS_RESULT(ok);

        ok = EELS_ERROR_OK;

        uint8_t calculatedCrc = EELS_CRC8(buf, this->raw_data_size);
        if (calculatedCrc != *slotcrc)
            ok = EELS_ERROR_NOK;
	}
    EELS_RESULT( ok );

	EELS_END();
}

EELSError _EELS_ReadLog(EELSh slotNumber, EELSAddr log_start_position, void* const dst){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    bool nolocked = EELS_Ready(slotNumber);

    EELSIndex ok = __EELS_ReadLog(this, log_start_position, dst);

    if ( EELS_Ready(slotNumber) && !nolocked ) // maybe should release always?
        // slot was busy and have finished op.
        EELS_RELEASE(slotNumber);

    return ok;
}

EELSEpoch _EELS_ReadCounter(EELSlot_t*  this, EELSAddr addr) {

    uint32_t ret_val = 0;

    //check with endianness and the way i write the counter bytes.
    EELSError ok = EELS_EEPROM_READ(addr, (uint8_t*)&ret_val, EELS_RECSIZE_EPOCH);
    if (ok >= 0)
        return ret_val;
    else
        return ok; //EELS_EPOCH_NONE;
}

EELSPosition _EELS_FindLastPos(EELSh slotNumber) {
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

    EELSPosition pos = eels_index_pos(this, 0);

    EELS_ASSERT( eels_pos_ok(this, pos) ) ;
    if ( !eels_pos_ok(this, pos) )
        return 0;

    EELSEpoch epoch = _EELS_ReadCounter(this, pos);   //current counter and position is SLOT_BEGIN

	//usually erased eeproms have [255] in their cells?
	if (( epoch & EELS_EPOCH_NONE) ) {
	    //if counter is invalid (bigger than max) - looks 1slot empty, and need start new epoch
	    epoch = next_epoch(epoch & ~EELS_EPOCH_NONE); //reset cnt so if next counter=1 it will be set ..
        
        this->overlaped         = false;
		this->epoch_counter     = epoch;
	    this->write_position    = pos;
	    this->write_index   = 0;
	    return pos;
	}

	this->epoch_counter = epoch;

    EELSPosition    hipos = eels_pred_pos(this, pos);

	EELSlotLen cnt = 0;

#if 0
	// search by linear records scan
	for (EELSPosition i= eels_next_pos(this, pos)
	        ; pos < i
	        ; pos = i, i= eels_next_pos(this, pos), ++cnt
	        )
	{
		tmp_cnt = _EELS_ReadCounter(slotNumber, i); //read the address

        if (tmp_cnt != epoch) // if (current = previous )
            break;
	}

#else

	// search by binary scan

    if (this->page_records > 0){
	    // binary search over pageX.[0] elements
	    EELSAddr        lp = eels_pos_page(this, pos);
	    EELSAddr        hp = eels_pos_page(this, hipos);
	    EELSAddr        xp = hp;

	    while ( lp != xp){
	        EELSPosition xpos = eels_pos(this, xp, this->page_offs);

	        EELSEpoch xepoch =  _EELS_ReadCounter(this, xpos);
	        if ( xepoch == epoch){
	            lp = xp;
	        }
	        else {
	            hp = xp-1;
	        }
	        if ( (hp-lp) > __page_size(this) )
	            xp = lp + eels_pos_page(this, (hp-lp)/2 );
	        else
	            break;
	    }
	    cnt = eels_page_idx(this, lp);

        xp = this->page_offs;
	    if (this->page_records > 1){
	        // binary search in page
	        unsigned li = 0;
	        unsigned hi = this->page_records-1;
	        unsigned xi = hi;

	        while(li != xi){
	            EELSPosition xpos = this->page_offs + this->slot_log_length * xi;
	            xpos = eels_pos(this, lp, xpos);

	            EELSEpoch xepoch =  _EELS_ReadCounter(this, xpos);
	            if ( xepoch == epoch){
	                li = xi;
	            }
	            else {
	                hi = xi-1;
	            }
                xi = li + (hi-li)/2;
	        }

	        xp = this->page_offs + this->slot_log_length * xi;
	        cnt += xi;
	    }

	    pos = eels_pos(this, lp, xp);
    }
    else {
        EELSlotLen      hi = this->_counter_max-1;
        EELSlotLen      xi = hi;

        while (cnt != xi){
            EELSPosition xpos = eels_index_pos(this, xi);

            EELSEpoch xepoch =  _EELS_ReadCounter(this, xpos);
            if ( xepoch == epoch){
                cnt = xi;
            }
            else {
                hi = xi;
            }
            xi = cnt + (hi-cnt)/2;
        }
        pos = eels_index_pos(this, cnt);
    }

#endif

	this->write_position    = pos;
    this->write_index   = cnt;

    //check that last written record is valid, if no - write from that one, else continue after that

    eels_write_advance(this);

    EELSEpoch hi_epoch = _EELS_ReadCounter(this, hipos);   //current counter and position is SLOT_BEGIN
    EELSEpoch last_epoch = _EELS_ReadCounter(this, this->write_position);   //current counter and position is SLOT_BEGIN

    // if record at write same epoch as topmost, assume buffer opverlaped
    this->overlaped         = (hi_epoch == last_epoch)              // pred epoch buffer is contigous
                            && is_next_epoch( hi_epoch, epoch )    // pred epoch exact previous
                            ;


    return pos;
}

uint16_t _EELS_getHealthyLogs(EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

#if _EELS_CRC_BYTE_LENGTH == 1
    uint8_t buf[256 /*this->slot_log_length*/ ];     // WARN: buffer on stack

	uint16_t healthyLogs = 0, curroptedLogs = 0;

    EELSPosition pos = eels_index_pos(this, 0);
    EELSPosition p = pos;
    for (EELSPosition i= eels_next_pos(this, p)
            ; p < i
            ; p = i, i= eels_next_pos(this, p)
            )
	{
        EELSError ok = _EELS_ReadLog(slotNumber, i, buf);
			if ( ok == EELS_ERROR_OK )
				healthyLogs++;
			else
				curroptedLogs++;
	}
	return healthyLogs;
#endif //_EELS_CRC_BYTE_LENGTH == 1
}

uint16_t _EELS_getHealthySequence(EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

#if _EELS_CRC_BYTE_LENGTH == 1

	    uint16_t sequences = 0;
		EELSEpoch counter = EELS_EPOCH_NONE-1;

        EELSPosition pos = eels_index_pos(this, 0);
		EELSPosition p = pos;
	    for (EELSPosition i= eels_next_pos(this, p)
	            ; p < i
	            ; p = i, i= eels_next_pos(this, p)
	            )
		{
			counter = _EELS_ReadCounter(this, i );
			if ( counter == this->epoch_counter )
			{
				sequences++;
				//printf("----_EELS_getHealthySequence [0x%.8X]: Counter:%d Prev:%u, seq:%d SL:%d\n", i ,counter,prev_counter,sequences, SL);
			}
		}

		return sequences;
#endif //_EELS_CRC_BYTE_LENGTH == 1
}

/* ==================================================================== */
/* 						DEBUG FUNCTIONS 								*/
/* ==================================================================== */


EELSDataLen EELS_SlotLogSize(EELSh slotNumber){
	return EELSlot(slotNumber)->slot_log_length;
}

EELSAddr EELS_SlotBegin(EELSh slotNumber){
	return EELSlot(slotNumber)->begining;
}

EELSAddr EELS_SlotEnd(EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);
	return this->begining + this->length;
}


uint8_t EELS_ReadCell(EELSAddr position){
	uint8_t result;
	EELS_EEPROM_READ(position, &result,1);
	return result;
}

void EELS_WriteCell(EELSAddr position, uint8_t val){
	EELS_EEPROM_WRITE(position, (uint8_t*)(&val), 1);
}


/// @brief check all active slots vs intersection
bool       EELS_validate_intersects(void){

    for (int i = 0; i < EELS_APP_SLOTS-1; ++i ){
        const EELSlot_t* prof = &(EELslot_arr[i]);

        if (prof->length <= 0) continue;
        if (EELS_SlotOnline(i) != EELS_ERROR_OK) continue;

        unsigned ilen = prof->page_limit;

        for (int j = i+1; j < EELS_APP_SLOTS; ++j )
        {
            const EELSlot_t* roomj = &EELslot_arr[j];

            if (roomj->length <= 0) continue;
            if (EELS_SlotOnline(j) != EELS_ERROR_OK) continue;

            if ( (roomj->begining >= (prof->begining + prof->length))
              || (prof->begining >= (roomj->begining + roomj->length)) )
                    continue;

            if ( (prof->page_records <= 0) || (roomj->page_records <= 0))
                // slot j - solid, not page_aligned, so it cant overlaps any slot
                return false;

            if (ilen <= 0)
                ilen = __page_size(prof);

            unsigned jlen = roomj->page_limit;
            if (jlen <= 0)
                jlen = __page_size(roomj);

            // pages are overlaped, check that sections not overlaps
            if ( (roomj->page_offs >= (prof->page_offs + ilen))
              || (prof->page_offs >= (roomj->page_offs + jlen)) )
                    continue;

            return false;
        }
    }
    return true;

}
