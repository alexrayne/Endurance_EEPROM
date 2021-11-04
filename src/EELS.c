/*
 * EELS.c
 *
 * Created: 7/24/2019 11:58:42 AM
 *  Author: hasan.jaafar
 */
#include <EELS_Conf.h>
#include "EELS.h"



EELSlot_t EELslot_arr[EELS_APP_SLOTS];




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
#define LOG2_1(n) (((n) >= 2u) ? 1u : 0u)
#define LOG2_2(n) (((n) >= 1u<<2) ? (2 + LOG2_1((n)>>2)) : LOG2_1(n))
#define LOG2_4(n) (((n) >= 1u<<4) ? (4 + LOG2_2((n)>>4)) : LOG2_2(n))
#define LOG2_8(n) (((n) >= 1u<<8) ? (8 + LOG2_4((n)>>8)) : LOG2_4(n))
#define LOG2_CEIL(n)   (((n) >= 1u<<16) ? (16 + LOG2_8((n)>>16)) : LOG2_8(n))
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

EELSEpoch _EELS_ReadCounter(EELSh slotNumber, uint32_t addr);
static EELSAddr    eels_pos_page(EELSlot_t*  this, EELSPosition x);

/* ==================================================================== */
/* 						PUBLIC FUNCTIONS 								*/
/* ==================================================================== */


void EELS_Init(){
    for (unsigned h = 0; h < EELS_APP_SLOTS; ++h) {
        EELSlot_t*  this = EELslot_arr + h;
        this->length            = 0;
        this->_counter_max      = 0;
        this->current_counter   = 0;
        this->slot_log_length   = 0;
        this->page_size         = 0;
        this->page_offs         = 0;
        this->page_limit        = 0;
    }
}

enum {
    EELS_RECOFFS_EPOCH  = 0,
    EELS_RECSIZE_EPOCH  = 1,

    EELS_RECOFFS_CRC    = EELS_RECOFFS_EPOCH + EELS_RECSIZE_EPOCH,
    EELS_RECSIZE_HEAD   = EELS_RECOFFS_CRC +_EELS_CRC_BYTE_LENGTH,

    EELS_RECOFFS_DATA   = EELS_RECSIZE_HEAD,

    EELS_EPOCH_NONE     = 0xff,
    EELS_EPOCH_MAX      = EELS_EPOCH_NONE-1,
};

EELSError EELS_SetSlot(EELSh slotNumber, EELSAddr begin_addr, uint16_t length, uint8_t data_length) {
	if (slotNumber >= EELS_APP_SLOTS)
		return EELS_ERROR_NOSLOT; //out of boundary!

	EELSlot_t*  this = EELSlot(slotNumber);

	/*user data length*/
	this->raw_data_size = data_length;
    /*Slot datalength*/
    this->slot_log_length = data_length + EELS_RECOFFS_DATA;

#ifndef EELS_PAGE_SIZE
    unsigned page_size = this->page_size;
    unsigned page_2pwr = this->page_2pwr;
#else
    enum {
        page_size = EELS_PAGE_SIZE,
        page_2pwr = LOG2_CEIL(EELS_PAGE_SIZE),
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

EELSError EELS_PageAlign  (EELSh slotNumber, EELSlotLen page_size){
    if (slotNumber >= EELS_APP_SLOTS)
        return EELS_ERROR_NOSLOT; //out of boundary!

    EELSlot_t*  this = EELSlot(slotNumber);

#ifndef EELS_PAGE_SIZE
    this->page_size     = page_size;
    this->page_2pwr     = eels_log2ceil(page_size);
#else
    enum ( page_size  = EELS_PAGE_SIZE, };
#endif
    this->page_offs     = 0;
    this->page_limit    = (EELSPageLen)page_size;

    return EELS_ERROR_OK;
}

EELSError EELS_PageSection(EELSh slotNumber, EELSPageLen page_offs, EELSPageLen sec_size){

    if (slotNumber >= EELS_APP_SLOTS)
        return EELS_ERROR_NOSLOT; //out of boundary!

    EELSlot_t*  this = EELSlot(slotNumber);

    if ( (page_offs + sec_size) > this->page_size)
        return  EELS_ERROR_SMALLPAGE;

    this->page_offs     = page_offs;
    this->page_limit    = page_offs + sec_size;

    return EELS_ERROR_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////
///         Position arithmetics

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
    return page + ofs;
}

static
unsigned    eels_page_idx(EELSlot_t*  this, EELSAddr page){
#ifdef EELS_PAGE_SIZE
    enum { page_2pwr = LOG2_CEIL(EELS_PAGE_SIZE) , };
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
        EELSAddr page = this->begining + this->page_size * pages;

        EELSAddr ofs  = this->page_offs;
        if (this->page_records > 1){
            unsigned  pidx  = idx - (pages * this->page_records);
            ofs  += (this->slot_log_length * pidx);
        }

        return eels_pos(this, page, ofs);
    }
}

static
EELSAddr    eels_relative_pos(EELSlot_t*  this, int idx){
    if ( idx >= this->_counter_max ) {
        return -1;  //tail
    }
    else if ( -idx >= this->_counter_max ) {
        return 0;   //head
    }
    // TODO: check actual slot fill size

    unsigned now_counter = this->current_counter;
    idx += now_counter;
    if (idx >= this->_counter_max)
        idx -= this->_counter_max;
    else if (idx < 0)
        idx += this->_counter_max;

    return  eels_index_pos(this, idx);
}

static
EELSPosition    eels_next_pos(EELSlot_t*  this, EELSPosition x){
    if (this->page_size <= 0) {
        x += this->slot_log_length;
        if (x >= (this->begining + this->length) )
            x = this->begining;
        return x;
    }

    EELSAddr page   = eels_pos_page(this, x);
    EELSAddr ofs    = eels_pos_ofs(this, x);

    EELSlotSize page_limit = this->page_size;
    if (this->page_limit > 0)
        page_limit = this->page_limit;

    ofs += this->slot_log_length;
    if ( (ofs + this->slot_log_length) > page_limit){
        ofs = this->page_offs;
        page += this->page_size;

        if ( (page + this->page_size) > (this->begining + this->length) )
            page = this->begining;
    }
    return eels_pos(this, page, ofs);
}

static
EELSPosition    eels_pred_pos(EELSlot_t*  this, EELSPosition x){
    if (this->page_size <= 0) {
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

        EELSlotSize page_limit = this->page_size;
        if (this->page_limit > 0)
            page_limit = this->page_limit;

        ofs = page_limit - this->slot_log_length;

        if (page < (this->begining + this->page_size) ) {
            page = (this->begining + this->length);
        }

        page -= this->page_size;
    }
    return eels_pos(this, page, ofs);
}



///////////////////////////////////////////////////////////////////////////////////////////

static
void eels_write_advance(EELSlot_t*  this){
    EELSPosition mypos= this->write_position;

    this->write_position = eels_next_pos(this, this->write_position);

    EELSlotLen cnt = this->current_counter;
    if (mypos > this->write_position)
        cnt = 0;
    else
        ++cnt;

    if (cnt >= this->_counter_max) {
        //if counter goes beyond counter max -> set back to 1
        cnt = 0;
        this->write_position = eels_index_pos(this, 0);
    }

    this->current_counter = cnt;
}

EELSIndex EELS_InsertLog(EELSh slotNumber, const void* src) {

	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	int ok;
    EELSlot_t*  this = EELSlot(slotNumber);
    const uint8_t* data = (const uint8_t*)src;

    EELSlotLen cnt    = this->current_counter;
    if (cnt >= this->_counter_max)
    //if (mypos > this->write_position)
    {
        // position wraped over slot length
        cnt = 0;
        this->current_counter = 0;
        this->write_position = eels_index_pos(this, 0);
    }
    EELSPosition mypos= this->write_position;

    if (cnt == 0){
        this->epoch_counter = _EELS_ReadCounter(slotNumber, mypos);
        ++this->epoch_counter;
        if (this->epoch_counter >= EELS_EPOCH_NONE)
            this->epoch_counter = 0;
    }

    /*=== LETS DO A WRITE ===*/

    char head[ EELS_RECSIZE_HEAD ];
	head[0] = this->epoch_counter;
	head[1] = EELS_CRC8(data, this->raw_data_size);

	ok = EELS_EEPROM_WRITE(mypos, head, sizeof(head) );
	if (ok < 0)
	    return ok;
	mypos += sizeof(head);

	ok = EELS_EEPROM_WRITE(mypos, data, this->raw_data_size);
    if (ok < 0)
        return ok;
	mypos += this->raw_data_size;

	#if (__EELS_DBG__)
		_EELS_FindLastPos(slotNumber);
	#else
		eels_write_advance(this);
	#endif

	return this->current_counter;
}


bool EELS_ReadLast(EELSh slotNumber, void* const buf){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	#if _EELS_CRC_BYTE_LENGTH == 1
		return _EELS_ReadLog(slotNumber, eels_pred_pos(this, this->write_position), buf);
	#endif

}



EELSPosition    EELS_PosIdx (EELSh slotNumber, int idx){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    return eels_relative_pos(this, idx);
}

EELSPosition    EELS_PosNext(EELSh slotNumber, EELSPosition from){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    return eels_next_pos(this, from);
}

bool            EELS_ReadPos(EELSh slotNumber, EELSPosition at , void* const buf ){
    return _EELS_ReadLog(slotNumber, at, buf);
}



bool    EELS_ReadIdx    (EELSh slotNumber, int log_num , void* const dst){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

    if ( log_num >= this->_counter_max ) {
        return false;  //tail
    }
    else if ( -log_num >= this->_counter_max ) {
        return false;   //head
    }

    #if (__EELS_DBG__)
    _EELS_FindLastPos(slotNumber);
    #endif

    uint32_t log_num_pos = eels_relative_pos(this, log_num);
    return _EELS_ReadLog(slotNumber, log_num_pos, dst);
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



bool _EELS_ReadLog(EELSh slotNumber, EELSAddr log_start_position, void* const dst){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
    uint8_t* buf = (uint8_t*)dst;

    uint8_t slotcrc=  0 ;
	EELS_EEPROM_READ(log_start_position + EELS_RECOFFS_CRC,
							(uint8_t*)&slotcrc, _EELS_CRC_BYTE_LENGTH );
	EELS_EEPROM_READ(log_start_position + EELS_RECOFFS_DATA ,
							buf, this->raw_data_size );
	uint8_t calculatedCrc = EELS_CRC8(buf, this->raw_data_size);
	return calculatedCrc == slotcrc;
}

EELSEpoch _EELS_ReadCounter(EELSh slotNumber, EELSAddr addr)
{
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

    uint32_t ret_val = 0;
	//check with endianness and the way i write the counter bytes.
	EELS_EEPROM_READ(addr, (uint8_t*)&ret_val, EELS_RECSIZE_EPOCH);

	return ret_val;
}

EELSAddr _EELS_FindLastPos(EELSh slotNumber) {
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

	EELSPosition pos = eels_index_pos(this, 0);
	EELSEpoch epoch = _EELS_ReadCounter(slotNumber, pos);   //current counter and position is SLOT_BEGIN

	//usually erased eeproms have [255] in their cells?
	if (epoch >= EELS_EPOCH_NONE) {
	    //if counter is invalid (bigger than max) - looks slot empty
	    epoch = 0; //reset cnt so if next counter=1 it will be set ..
		this->epoch_counter     = 0;
	    this->write_position    = pos;
	    this->current_counter   = 0;
		return pos;
	}

	this->epoch_counter = epoch;

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
    EELSPosition    hipos = eels_pred_pos(this, pos);

    if (this->page_records > 0){
	    // binary search over pageX.[0] elements
	    EELSAddr        lp = eels_pos_page(this, pos);
	    EELSAddr        hp = eels_pos_page(this, hipos);
	    EELSAddr        xp = hp;

	    while ( lp != xp){
	        EELSPosition xpos = eels_pos(this, xp, this->page_offs);

	        EELSEpoch xepoch =  _EELS_ReadCounter(slotNumber, xpos);
	        if ( xepoch == epoch){
	            lp = xp;
	        }
	        else {
	            hp = xp;
	        }
	        if ( (hp-lp) > this->page_size )
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

	            EELSEpoch xepoch =  _EELS_ReadCounter(slotNumber, xpos);
	            if ( xepoch == epoch){
	                li = xi;
	            }
	            else {
	                hi = xi;
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

            EELSEpoch xepoch =  _EELS_ReadCounter(slotNumber, xpos);
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
    this->current_counter   = cnt;
    eels_write_advance(this);

    return cnt; //this->current_counter;
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
			if ( _EELS_ReadLog(slotNumber, i, buf) )
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
			counter = _EELS_ReadCounter(slotNumber, i );
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
