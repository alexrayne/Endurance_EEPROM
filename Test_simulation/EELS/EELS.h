#pragma once

/*
* EELS.h
*
* Created: 05/2/2019 3:08:44 PM
* Author: hasan.jaafar
*/

//#include "driver\E24LC64.h"

#include <stdafx.h>


#include "EELS_EEPROM_Interface.h"

#include <cmath>
#define EELS_MAX_SLOTS 5

#define _EELS_CRC_BYTE_LENGTH 1




//enum{SUCCESS, ERR_FAIL, ERR_MAX_SLOTS, MEMORY_ALLOCATION} EELS_RETURN_TYPE;

/* EEPROM LOG SYSTEM */

class EELS: public eels_eeprom
{
public:
	typedef struct {
		uint16_t begining;
		uint16_t length;
		uint16_t current_position;
		uint16_t current_counter;
		uint8_t data_length;
		uint8_t raw_data_size;
		uint16_t _counter_max;
		uint8_t _counter_bytes;
	}ee_slot_t;
//variables
public:
protected:
private:
	ee_slot_t * _myslots;
	uint8_t _NumOfSlot;
	void* _slot_arr[EELS_MAX_SLOTS];

	//functions
public:
	EELS(Twi* pTWI, uint8_t addr,uint8_t slots);
	void InsertLog(uint8_t slotNumber, uint8_t* data);
	uint8_t SetSlot(uint8_t slotNumber, uint16_t begin_addr, uint16_t length, uint8_t data_length);
	uint16_t GetLogPos(uint8_t slotNumber);
	uint16_t GetCounter(uint8_t slotNumber);
	uint16_t GetLogSize(uint8_t slotNumber);
	uint16_t GetLastPos(uint8_t slotNumber);
	ee_slot_t& operator[](uint8_t i);//get slot
	uint8_t ReadEeprom(uint16_t addr);
	void WriteEeprom(uint16_t addr, uint8_t val);
	~EELS();
protected:
private:
	int32_t _FindLastPos(uint8_t slotNumber);
	//E24LC64 * _eeprom_obj;
	uint16_t _SlotCntMax(uint16_t length, uint8_t data_length);
	uint32_t _ReadAddr(uint8_t slotNumber, uint16_t addr);
}; //EELS
