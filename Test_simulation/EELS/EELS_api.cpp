/*
* EELS.cpp
*
* Created: 05/2/2019 3:08:44 PM
* Author: hasan.jaafar
*/

#include "EELS.hpp"
#include "EELS_Conf.h"


// default constructor
EELS::EELS(Twi* pTWI, uint8_t addr, uint8_t slots)
{
	eels_eeprom_init(pTWI, addr);
	//_eeprom_obj = eeprom;
}


uint8_t EELS::SetSlot(uint8_t slotNumber, uint16_t begin_addr, uint16_t length, uint8_t data_length) {
    return EELS_SetSlot(slotNumber, begin_addr, length, data_length);
}

EELSError EELS::PageAlign(EELSh slotNumber, EELSlotLen page_size){
    return EELS_PageAlign(slotNumber, page_size);
}

EELSError EELS::PageSection(EELSh slotNumber, EELSPageLen page_offs, EELSPageLen sec_size){
    return EELS_PageSection(slotNumber, page_offs, sec_size);
}

uint8_t EELS::ReadEeprom(uint16_t addr) {
	uint8_t ret_val;
	_eeprom_obj->eeprom_read(addr, &ret_val);
	return ret_val;
}

void EELS::WriteEeprom(uint16_t addr, uint8_t val) {
	_eeprom_obj->eeprom_write(addr,val);
}

EELS::ee_slot_t& EELS::operator[](uint8_t i) {
    if (i >= EELS_APP_SLOTS)
        return *( EELSlot(0) );
    return *EELSlot(i);
}


uint16_t EELS::GetLogPos(uint8_t slotNumber) {
	return (*this)[slotNumber].write_position;
}
uint16_t EELS::GetCounter(uint8_t slotNumber) {
	return (*this)[slotNumber].write_index;
}

uint16_t EELS::GetLogSize(uint8_t slotNumber) {
	return (*this)[slotNumber].slot_log_length;
}

uint16_t EELS::GetLastPos(uint8_t slotNumber) {
	return _EELS_FindLastPos(slotNumber);
}

uint16_t EELS::PageSize(uint8_t slotNumber){
    return (*this)[slotNumber].page_size;
}

void EELS::InsertLog(uint8_t slotNumber, uint8_t* data) {
    EELS_InsertLog(slotNumber, data);
}

void EELS::CleanLog(uint8_t slotNumber) {
	EELS_CleanLog(slotNumber);
}

// default destructor
EELS::~EELS()
{
}
