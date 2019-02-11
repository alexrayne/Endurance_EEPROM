/*
* EELS.cpp
*
* Created: 05/2/2019 3:08:44 PM
* Author: hasan.jaafar
*/

#include <stdafx.h>

#include "EELS.h"

// default constructor
EELS::EELS(Twi* pTWI, uint8_t addr, uint8_t slots)
		: _NumOfSlot(slots)
{
	eels_eeprom_init(pTWI, addr);
	//_eeprom_obj = eeprom;
	_myslots = new ee_slot_t[_NumOfSlot];
}


uint8_t EELS::SetSlot(uint8_t slotNumber, uint16_t begin_addr, uint16_t length, uint8_t data_length) {
	if (slotNumber >= _NumOfSlot)
		return 1; //out of boundary!
	_myslots[slotNumber].begining = begin_addr;
	_myslots[slotNumber].length = length;
	/*user data length*/
	_myslots[slotNumber].raw_data_size = data_length;
	/*calculate initial counter max*/ // +1 at least 1 byte for counter
	uint16_t init_counter_max = (uint16_t)std::ceil(length / data_length + _EELS_CRC_BYTE_LENGTH+1) + 2; //always add +2 extra
	uint8_t bits_required = (uint8_t)std::ceil(log2(init_counter_max));
	_myslots[slotNumber]._counter_bytes = (uint8_t)std::ceil(bits_required / 8.0);
	/*Slot datalength*/
	_myslots[slotNumber].data_length = data_length + _EELS_CRC_BYTE_LENGTH + _myslots[slotNumber]._counter_bytes;
	/*Recalculate counter max*/
	_myslots[slotNumber]._counter_max = (uint16_t)std::ceil(length / _myslots[slotNumber].data_length)+2;

	_FindLastPos(slotNumber);
	printf("Max counter: %d", _myslots[slotNumber]._counter_max);
	printf(" - counter bytes: %d\r\n", _myslots[slotNumber]._counter_bytes);

	//.eeprom_write(0, addrzero, 4)
	return 0;
}

EELS::ee_slot_t& EELS::operator[](uint8_t i) {
	if (i > _NumOfSlot - 1)
		return _myslots[0];
	return _myslots[i];
}

uint8_t EELS::ReadEeprom(uint16_t addr) {
	uint8_t ret_val;
	_eeprom_obj->eeprom_read(addr, &ret_val);
	return ret_val;
}

void EELS::WriteEeprom(uint16_t addr, uint8_t val) {
	_eeprom_obj->eeprom_write(addr,val);
}

uint16_t EELS::GetLogPos(uint8_t slotNumber) {
	return _myslots[slotNumber].current_position;
}
uint16_t EELS::GetCounter(uint8_t slotNumber) {
	return _myslots[slotNumber].current_counter;
}

uint16_t EELS::GetLogSize(uint8_t slotNumber) {
	return _myslots[slotNumber].data_length;
}

uint16_t EELS::GetLastPos(uint8_t slotNumber) {
	return _FindLastPos(slotNumber);
}

void EELS::InsertLog(uint8_t slotNumber, uint8_t* data) {
	#define __EELS_DBG__ (1)
	#if __EELS_DBG__
		_FindLastPos(slotNumber);
	#endif
	uint32_t cnt = _myslots[slotNumber].current_counter;
	uint16_t mypos = _myslots[slotNumber].current_position;
	uint16_t S_begin = _myslots[slotNumber].begining;

	//-------- instead of this: lets make proper last pos identifier:
	if (!(_myslots[slotNumber].current_position == S_begin && _myslots[slotNumber].current_counter == 0)) // put description of this situation...
		mypos += _myslots[slotNumber].data_length;
	//=================================================
	if (++cnt > _myslots[slotNumber]._counter_max) //if counter goes beyond counter max -> set back to 1
		cnt = 1;
	printf("mypos+S_begin:%d\r\n", mypos + _myslots[slotNumber].data_length);
	printf("S(%d).length = :%d\r\n", slotNumber,  _myslots[slotNumber].length);
	if ((mypos + _myslots[slotNumber].data_length) > S_begin + _myslots[slotNumber].length) // set position to slot begining
		mypos = S_begin;
	
	/*=== LETS DO A WRITE ===*/
	char char_cnt[4] = { char(cnt), char(cnt >> 8), char(cnt >> 16), char(cnt >> 24) };
	_eeprom_obj->eeprom_write(mypos, (uint8_t*)char_cnt, _myslots[slotNumber]._counter_bytes);
	mypos += _myslots[slotNumber]._counter_bytes;
	uint32_t mycrc = 0xddffffff;
	_eeprom_obj->eeprom_write(mypos, (uint8_t*)(&mycrc), _EELS_CRC_BYTE_LENGTH);
	mypos += _EELS_CRC_BYTE_LENGTH;
	/*=== user data ===*/
	for (int i = 0; i < _myslots[slotNumber].raw_data_size; i++) {
		_eeprom_obj->eeprom_write(mypos++, data[i]);
	}
	
	_myslots[slotNumber].current_counter = cnt;
	_myslots[slotNumber].current_position = mypos;

	#if __EELS_DBG__
	_FindLastPos(slotNumber);
	#endif
}

int32_t EELS::_FindLastPos(uint8_t slotNumber) {
	uint16_t _counter_max = _myslots[slotNumber]._counter_max; //don't use 0s. although the EEPROM comes with 0xff filled
	uint16_t SDBL = _myslots[slotNumber].data_length; //slot data byte length
	uint16_t S_begin = _myslots[slotNumber].begining;
	uint16_t length = _myslots[slotNumber].length;
	
	uint32_t pos = S_begin;
	uint32_t prev_cnt = _ReadAddr(slotNumber, S_begin);
	uint32_t tmp_cnt = 0;

	for (uint32_t i=(S_begin+ SDBL); i<(S_begin + length); i+=SDBL) {
		if (i + SDBL > S_begin+length)
			continue;
		tmp_cnt = _ReadAddr(slotNumber, i); //read the address

		if (tmp_cnt == prev_cnt + 1){// if (current = previous +1)
			prev_cnt = tmp_cnt;
			pos = i;
		}else if (tmp_cnt == 1 && prev_cnt == _counter_max) { //if (previous == max AND current == 1)
			prev_cnt = tmp_cnt;
			pos = i;
		}
	}
	_myslots[slotNumber].current_position = pos;
	_myslots[slotNumber].current_counter = prev_cnt;
	return pos;
}


uint32_t EELS::_ReadAddr(uint8_t slotNumber, uint16_t addr)
{
	uint32_t ret_val = 0;
	uint8_t buff;
	_eeprom_obj->eeprom_read(addr, (uint8_t*)&ret_val, _myslots[slotNumber]._counter_bytes);
	/*for (int i = 0; i < _myslots[slotNumber]._counter_bytes ; i++) {
		ret_val <<= 8;
		_eeprom_obj->eeprom_read(addr, &buff);
		ret_val |= buff;
	}
	printf("EELS::_ReadAddr(%d)", ret_val);*/
	return ret_val;
}

uint16_t EELS::_SlotCntMax(uint16_t length, uint8_t raw_data_length) {
	raw_data_length += _EELS_CRC_BYTE_LENGTH + 1; //at least one byte for log address byte
	return (uint16_t)std::ceil(length / raw_data_length) + 2;
}



// default destructor
EELS::~EELS()
{
	delete[] _myslots;
}
