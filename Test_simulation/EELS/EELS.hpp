#pragma once

/*
* EELS.h
*
* Created: 05/2/2019 3:08:44 PM
* Author: hasan.jaafar
*/

//#include "driver\E24LC64.h"

#include "EELS_EEPROM_Interface.h"
#include <EELS.h>



//enum{SUCCESS, ERR_FAIL, ERR_MAX_SLOTS, MEMORY_ALLOCATION} EELS_RETURN_TYPE;

/* EEPROM LOG SYSTEM */

class EELS: public eels_eeprom
{
public:
	typedef EELSlot_t ee_slot_t;
//variables
public:
protected:
private:
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
	//E24LC64 * _eeprom_obj;
}; //EELS
