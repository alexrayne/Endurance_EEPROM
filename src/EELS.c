/*
 * EELS.c
 *
 * Created: 7/24/2019 11:58:42 AM
 *  Author: hasan.jaafar
 */
#include "EELS_Conf.h"
#include "EELS.h"
#include <math.h>


uint32_t _EELS_ReadCounter(uint8_t slotNumber, uint32_t addr);

uint8_t _NumOfSlot;
ee_slot_t _slot_arr[EELS_APP_SLOTS];


/* ==================================================================== */
/* 						PUBLIC FUNCTIONS 								*/
/* ==================================================================== */


uint8_t EELS_Init(){
	return 0;
}



uint8_t EELS_SetSlot(uint8_t slotNumber, uint32_t begin_addr, uint16_t length, uint8_t data_length) {
	if (slotNumber >= EELS_APP_SLOTS)
		return 1; //out of boundary!
	_slot_arr[slotNumber].begining = begin_addr;
	_slot_arr[slotNumber].length = length;
	/*user data length*/
	_slot_arr[slotNumber].raw_data_size = data_length;
	/*calculate initial counter max*/ // +1 at least 1 byte for counter
	uint16_t init_counter_max = (uint16_t)ceil(length / data_length + _EELS_CRC_BYTE_LENGTH+1) + 2; //always add +2 extra
	uint8_t bits_required = (uint8_t)ceil(log2(init_counter_max));
	_slot_arr[slotNumber]._counter_bytes = (uint8_t)ceil(bits_required / 8.0f);
	/*Slot datalength*/
	_slot_arr[slotNumber].slot_log_length = data_length + _EELS_CRC_BYTE_LENGTH + _slot_arr[slotNumber]._counter_bytes;
	/*Recalculate counter max*/
	_slot_arr[slotNumber]._counter_max = (uint16_t)ceil(length / _slot_arr[slotNumber].slot_log_length)+2; //readjust max counter

	_EELS_FindLastPos(slotNumber); //now
	//printf("Max counter: %d", _slot_arr[slotNumber]._counter_max);
	//printf(" - counter bytes: %d\r\n", _slot_arr[slotNumber]._counter_bytes);

	return 0;
}


void EELS_InsertLog(uint8_t slotNumber, uint8_t* data) {

	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif
	uint32_t cnt = _slot_arr[slotNumber].current_counter;
	uint32_t mypos = _slot_arr[slotNumber].current_position;
	uint32_t S_begin = _slot_arr[slotNumber].begining;

	//-------- instead of this: lets make proper last pos identifier:
	if (!(_slot_arr[slotNumber].current_position == S_begin && _slot_arr[slotNumber].current_counter == 0)) // put description of this situation...
		mypos += _slot_arr[slotNumber].slot_log_length;
	//--------
	if (++cnt > _slot_arr[slotNumber]._counter_max) //if counter goes beyond counter max -> set back to 1
		cnt = 1;

	if ((mypos + _slot_arr[slotNumber].slot_log_length) > S_begin + _slot_arr[slotNumber].length) // set position to slot begining
	mypos = S_begin;

	/*=== LETS DO A WRITE ===*/
	char char_cnt[4] = { (char)(cnt), (char)(cnt >> 8), (char)(cnt >> 16), (char)(cnt >> 24) };
	EELS_EEPROM_WRITE(mypos, (uint8_t*)char_cnt, _slot_arr[slotNumber]._counter_bytes);
	mypos += _slot_arr[slotNumber]._counter_bytes;
	uint8_t mycrc = EELS_CRC8(data, _slot_arr[slotNumber].raw_data_size);
	EELS_EEPROM_WRITE(mypos, (uint8_t*)(&mycrc), _EELS_CRC_BYTE_LENGTH);
	mypos += _EELS_CRC_BYTE_LENGTH;
	EELS_EEPROM_WRITE(mypos, data,_slot_arr[slotNumber].raw_data_size);
	mypos += _slot_arr[slotNumber].raw_data_size;

	#if (__EELS_DBG__)
		_EELS_FindLastPos(slotNumber);
	#else
		_slot_arr[slotNumber].current_counter = cnt;
		_slot_arr[slotNumber].current_position = mypos -  _slot_arr[slotNumber].slot_log_length;
	#endif
}


bool EELS_ReadLast(uint8_t slotNumber, uint8_t* const buf){
	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	#if _EELS_CRC_BYTE_LENGTH == 1
		return _EELS_ReadLog(slotNumber, _slot_arr[slotNumber].current_position, buf);
	#endif

}


/*under development*/
bool EELS_ReadFromEnd(uint8_t slotNumber, uint16_t log_num , uint8_t* const buf ){
	if (log_num >= _slot_arr[slotNumber]._counter_max){ //cant read > slot size
		return 0;
	}
	#if (__EELS_DBG__)
	_EELS_FindLastPos(slotNumber);
	#endif

	uint32_t last_log_pos = _slot_arr[slotNumber].current_position;
	uint32_t last_log_counter = _slot_arr[slotNumber].current_counter;

	uint32_t diff_length = (_slot_arr[slotNumber].slot_log_length * log_num);
	uint32_t padding_bytes = _slot_arr[slotNumber].length - ((_slot_arr[slotNumber]._counter_max-1)*_slot_arr[slotNumber].slot_log_length);
	//printf("\t diff_length=%u \t padding_bytes=%d\n", diff_length, padding_bytes);
	if (diff_length < last_log_pos){
		uint32_t log_num_pos = last_log_pos - diff_length;
		//printf("\t log_num_pos=0x%x \t padding_bytes=%d\n", log_num_pos, padding_bytes);
		if (log_num_pos >= _slot_arr[slotNumber].begining){
				return _EELS_ReadLog(slotNumber, log_num_pos, buf);
		}else{
				log_num_pos += (_slot_arr[slotNumber].length - padding_bytes - _slot_arr[slotNumber].slot_log_length );
				return _EELS_ReadLog(slotNumber, log_num_pos, buf);
				//uint32_t slot_last_addr = _slot_arr[slotNumber].begining + ((_slot_arr[slotNumber]._counter_max - 2 ) * _slot_arr[slotNumber].slot_log_length);
		}

	}
	return 0; //should never reach here.
}



/* ==================================================================== */
/* 						PRIVATE FUNCTIONS 								*/
/* ==================================================================== */

#if EELS_LOCAL_CRC_FN

uint8_t EELS_crc8(uint8_t *data, uint8_t len)
{
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

#endif



bool _EELS_ReadLog(uint8_t slotNumber, uint32_t log_start_position, uint8_t* const buf){
	uint8_t slotcrc=  0 ;
	EELS_EEPROM_READ(log_start_position + _slot_arr[slotNumber]._counter_bytes ,
							(uint8_t*)&slotcrc,
							_EELS_CRC_BYTE_LENGTH);
	EELS_EEPROM_READ(log_start_position + _slot_arr[slotNumber]._counter_bytes + _EELS_CRC_BYTE_LENGTH ,
							buf,
							_slot_arr[slotNumber].raw_data_size);
	uint8_t calculatedCrc = EELS_CRC8(buf,_slot_arr[slotNumber].raw_data_size);
	return calculatedCrc == slotcrc;
}

uint32_t _EELS_ReadCounter(uint8_t slotNumber, uint32_t addr)
{
	uint32_t ret_val = 0;
	//check with endianness and the way i write the counter bytes.
	EELS_EEPROM_READ(addr, (uint8_t*)&ret_val, _slot_arr[slotNumber]._counter_bytes);

	return ret_val;
}

uint32_t _EELS_FindLastPos(uint8_t slotNumber) {
	uint16_t _counter_max = _slot_arr[slotNumber]._counter_max; //don't use 0s. although the EEPROM comes with 0xff filled
	uint16_t SL = _slot_arr[slotNumber].slot_log_length;
	uint32_t S_begin = _slot_arr[slotNumber].begining;
	uint16_t length = _slot_arr[slotNumber].length;

	uint32_t pos = S_begin;
	uint32_t cnt = _EELS_ReadCounter(slotNumber, S_begin);   //current counter and position is SLOT_BEGIN
	//usually erased eeproms have [255] in their cells?
	if (cnt > _slot_arr[slotNumber]._counter_max) //if counter is invalid (bigger than max)
		cnt = 0; //reset cnt so if next counter=1 it will be set ..
	uint32_t tmp_cnt = 0;

	for (uint32_t i=(S_begin+ SL); i<(S_begin + length); i+=SL) {
		if (i + SL > S_begin+length)
		continue;
		tmp_cnt = _EELS_ReadCounter(slotNumber, i); //read the address

		if (tmp_cnt == cnt + 1){// if (current = previous +1)
			cnt = tmp_cnt;
			pos = i;
		}else if (tmp_cnt == 1 && cnt == _counter_max) { //if (previous == max AND current == 1)
			cnt = tmp_cnt;
			pos = i;
		}
	}
	if (cnt > _slot_arr[slotNumber]._counter_max)
		cnt = 1;
	_slot_arr[slotNumber].current_position = pos;
	_slot_arr[slotNumber].current_counter = cnt;
	return pos;
}

uint16_t _EELS_getHealthyLogs(uint8_t slotNumber){

	#if _EELS_CRC_BYTE_LENGTH == 1
	uint32_t S_begin = _slot_arr[slotNumber].begining;
	uint16_t SL = _slot_arr[slotNumber].slot_log_length;
	uint16_t length = _slot_arr[slotNumber].length;
	uint8_t buf[_slot_arr[slotNumber].slot_log_length];

	uint16_t healthyLogs = 0, curroptedLogs = 0;

	for (uint32_t i=(S_begin); i<(S_begin + length); i+=SL) {
			uint8_t LogCRC=  0 ;
			EELS_EEPROM_READ(i + _slot_arr[slotNumber]._counter_bytes,
									 (uint8_t*)&LogCRC,
									 _EELS_CRC_BYTE_LENGTH);
			EELS_EEPROM_READ(i + _slot_arr[slotNumber]._counter_bytes + _EELS_CRC_BYTE_LENGTH ,
									buf,
									_slot_arr[slotNumber].raw_data_size);
			uint8_t calculatedCrc = EELS_CRC8(buf,_slot_arr[slotNumber].raw_data_size);
			if (calculatedCrc == LogCRC)
				healthyLogs++;
			else
				curroptedLogs++;
	}
	return healthyLogs;
	#endif
}

uint16_t _EELS_getHealthySequence(uint8_t slotNumber){
		#if _EELS_CRC_BYTE_LENGTH == 1
		uint32_t S_begin = _slot_arr[slotNumber].begining;
		uint16_t SL = _slot_arr[slotNumber].slot_log_length;
		uint16_t length = _slot_arr[slotNumber].length;
		uint8_t buf[_slot_arr[slotNumber].slot_log_length];

		uint16_t sequences = 0;
		uint32_t counter, prev_counter = UINT_MAX-1;
		for(uint32_t i=(S_begin); i<(S_begin+length); i+= SL){
			if (S_begin+length < i+SL)
				continue;
			counter = _EELS_ReadCounter(slotNumber, i );
			if ((counter != prev_counter + 1)  && !(counter == 1 && prev_counter == _slot_arr[slotNumber]._counter_max) ){
				sequences++;
				//printf("----_EELS_getHealthySequence [0x%.8X]: Counter:%d Prev:%u, seq:%d SL:%d\n", i ,counter,prev_counter,sequences, SL);
			}
			prev_counter = counter;

		}

		return sequences;
		#endif
}

/* ==================================================================== */
/* 						DEBUG FUNCTIONS 								*/
/* ==================================================================== */


uint8_t EELS_SlotLogSize(uint8_t slotNumber){
	return _slot_arr[slotNumber].slot_log_length;
}

uint32_t EELS_SlotBegin(uint8_t slotNumber){
	return _slot_arr[slotNumber].begining;
}

uint32_t EELS_SlotEnd(uint8_t slotNumber){
	return _slot_arr[slotNumber].begining + _slot_arr[slotNumber].length;
}


uint8_t EELS_ReadCell(uint32_t position){
	uint8_t result;
	EELS_EEPROM_READ(position, &result,1);
	return result;
}

void EELS_WriteCell(uint32_t position, uint8_t val){
	EELS_EEPROM_WRITE(position, (uint8_t*)(&val), 1);
}
