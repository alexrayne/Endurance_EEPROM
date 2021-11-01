/*
 * EELS.c
 *
 * Created: 7/24/2019 11:58:42 AM
 *  Author: hasan.jaafar
 */
#include "EELS_Conf.h"
#include "EELS.h"


EELSEpoch _EELS_ReadCounter(EELSh slotNumber, uint32_t addr);

EELSh       _NumOfSlot;
EELSlot_t EELslot_arr[EELS_APP_SLOTS];


/* ==================================================================== */
/* 						PUBLIC FUNCTIONS 								*/
/* ==================================================================== */


uint8_t EELS_Init(){
	return 0;
}

enum {
    EELS_RECOFFS_EPOCH  = 0,
    EELS_RECSIZE_EPOCH  = 1,

    EELS_RECOFFS_CRC    = EELS_RECOFFS_EPOCH + EELS_RECSIZE_EPOCH,

    EELS_RECOFFS_DATA   = EELS_RECOFFS_CRC +_EELS_CRC_BYTE_LENGTH,

    EELS_EPOCH_NONE     = 0xff,
    EELS_EPOCH_MAX      = EELS_EPOCH_NONE-1,
};


uint8_t EELS_SetSlot(EELSh slotNumber, uint32_t begin_addr, uint16_t length, uint8_t data_length) {
	if (slotNumber >= EELS_APP_SLOTS)
		return 1; //out of boundary!

	EELSlot_t*  this = EELSlot(slotNumber);

	this->begining      = begin_addr;
	this->length        = length;

	/*user data length*/
	this->raw_data_size = data_length;
    /*Slot datalength*/
    this->slot_log_length = data_length + EELS_RECOFFS_DATA;

    this->_counter_max = (length / this->slot_log_length); //readjust max counter

	_EELS_FindLastPos(slotNumber); //now
	//printf("Max counter: %d", _slot_arr[slotNumber]._counter_max);
	//printf(" - counter bytes: %d\r\n", _slot_arr[slotNumber]._counter_bytes);

	return 0;
}


int EELS_InsertLog(EELSh slotNumber, const void* src) {

	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	int ok;
    EELSlot_t*  this = EELSlot(slotNumber);
    const uint8_t* data = (const uint8_t*)src;

	uint32_t cnt    = this->current_counter;
	uint32_t mypos  = this->current_position;
	uint32_t S_begin= this->begining;

	//-------- instead of this: lets make proper last pos identifier:
	if (!(this->current_position == S_begin && this->current_counter == 0)) // put description of this situation...
		mypos += this->slot_log_length;
	//--------
	if (++cnt > this->_counter_max) {//if counter goes beyond counter max -> set back to 1
        cnt = 1;
        ++this->epoch_counter;
        if (this->epoch_counter >= EELS_EPOCH_NONE)
            this->epoch_counter = 0;
	}

	if ((mypos + this->slot_log_length) > S_begin + this->length) // set position to slot begining
	    mypos = S_begin;

	/*=== LETS DO A WRITE ===*/
	char head[ EELS_RECOFFS_DATA ];
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
		this->current_counter = cnt;
		this->current_position = mypos -  this->slot_log_length;
	#endif

	return cnt;
}


bool EELS_ReadLast(EELSh slotNumber, void* const buf){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;
	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	#if _EELS_CRC_BYTE_LENGTH == 1
		return _EELS_ReadLog(slotNumber, this->current_position, buf);
	#endif

}


static
uint32_t    eels_index_pos(EELSlot_t*  this, int idx){
    if ( idx >= this->_counter_max ) {
        return -1;  //tail
    }
    else if ( -idx >= this->_counter_max ) {
        return 0;   //head
    }
    // TODO: check actual slot fill size

    uint32_t now_counter   = this->current_counter;
    idx += now_counter;
    if (idx >= this->_counter_max)
        idx -= this->_counter_max;
    else if (idx < 0)
        idx += this->_counter_max;

    return this->begining + (this->slot_log_length * idx);
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

    uint32_t log_num_pos = eels_index_pos(log_num);
    return _EELS_ReadLog(slotNumber, log_num_pos, dst);
}



/* ==================================================================== */
/* 						PRIVATE FUNCTIONS 								*/
/* ==================================================================== */

uint8_t EELS_crc8(const void *src, uint8_t len)
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



bool _EELS_ReadLog(EELSh slotNumber, uint32_t log_start_position, void* const dst){
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

EELSEpoch _EELS_ReadCounter(EELSh slotNumber, uint32_t addr)
{
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

    uint32_t ret_val = 0;
	//check with endianness and the way i write the counter bytes.
	EELS_EEPROM_READ(addr, (uint8_t*)&ret_val, EELS_RECSIZE_EPOCH);

	return ret_val;
}

uint32_t _EELS_FindLastPos(EELSh slotNumber) {
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

    uint16_t _counter_max   = this->_counter_max; //don't use 0s. although the EEPROM comes with 0xff filled
	uint16_t SL             = this->slot_log_length;
	uint32_t S_begin        = this->begining;
	uint16_t length         = this->length;

	uint32_t pos = S_begin;
	EELSEpoch cnt = _EELS_ReadCounter(slotNumber, S_begin);   //current counter and position is SLOT_BEGIN

	//usually erased eeproms have [255] in their cells?
	if (cnt >= EELS_EPOCH_NONE) {
	    //if counter is invalid (bigger than max) - looks slot empty
		cnt = 0; //reset cnt so if next counter=1 it will be set ..
		this->epoch_counter = 0;
	    this->current_position = pos;
	    this->current_counter = cnt;
		return pos;
	}

	this->epoch_counter = cnt;

	EELSEpoch tmp_cnt;

	for (uint32_t i=(S_begin+ SL); i<(S_begin + length); i+=SL) {
		if (i + SL > S_begin+length)   break;

		tmp_cnt = _EELS_ReadCounter(slotNumber, i); //read the address

		if (tmp_cnt == cnt){// if (current = previous )
			pos = i;
		}
	}
	this->current_position = pos;
	this->current_counter = cnt;
	return pos;
}

uint16_t _EELS_getHealthyLogs(EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);     (void)this;

#if _EELS_CRC_BYTE_LENGTH == 1
	uint32_t S_begin    = this->begining;
	uint16_t SL         = this->slot_log_length;
	uint16_t length     = this->length;
	uint8_t buf[this->slot_log_length];     // WARN: buffer on stack

	uint16_t healthyLogs = 0, curroptedLogs = 0;

	for (uint32_t i=(S_begin); i<(S_begin + length); i+=SL) {
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
		uint32_t S_begin    = this->begining;
		uint16_t SL         = this->slot_log_length;
		uint16_t length     = this->length;
		uint8_t buf[this->slot_log_length]; // WARN: buffer on stack

		uint16_t sequences = 0;
		uint32_t counter, prev_counter = UINT_MAX-1;
		for(uint32_t i=(S_begin); i<(S_begin+length); i+= SL){
			if (S_begin+length < i+SL)
				continue;
			counter = _EELS_ReadCounter(slotNumber, i );
			if ( (counter != prev_counter + 1)
			    && !(counter == 1 && prev_counter == this->_counter_max)
			    )
			{
				sequences++;
				//printf("----_EELS_getHealthySequence [0x%.8X]: Counter:%d Prev:%u, seq:%d SL:%d\n", i ,counter,prev_counter,sequences, SL);
			}
			prev_counter = counter;

		}

		return sequences;
#endif //_EELS_CRC_BYTE_LENGTH == 1
}

/* ==================================================================== */
/* 						DEBUG FUNCTIONS 								*/
/* ==================================================================== */


uint8_t EELS_SlotLogSize(EELSh slotNumber){
	return EELSlot(slotNumber)->slot_log_length;
}

uint32_t EELS_SlotBegin(EELSh slotNumber){
	return EELSlot(slotNumber)->begining;
}

uint32_t EELS_SlotEnd(EELSh slotNumber){
    EELSlot_t*  this = EELSlot(slotNumber);
	return this->begining + this->length;
}


uint8_t EELS_ReadCell(uint32_t position){
	uint8_t result;
	EELS_EEPROM_READ(position, &result,1);
	return result;
}

void EELS_WriteCell(uint32_t position, uint8_t val){
	EELS_EEPROM_WRITE(position, (uint8_t*)(&val), 1);
}
