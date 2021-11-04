#include "stdafx.h"
#include "EELS_EEPROM_Interface.h"


E24LC64* eels_eeprom::_eeprom_obj = nullptr;

void eels_eeprom::eels_eeprom_init(Twi* pTWI, uint8_t addr)
{
	#ifdef EELS_24LC64
		_eeprom_obj = new E24LC64(pTWI, addr);
	#else
		#error DEFINE SUPPORTED EEPROM CHIP
	#endif
}

eels_eeprom::~eels_eeprom()
{
	delete _eeprom_obj;
}
/*

EELS_EEPROM_Interface::EELS_EEPROM_Interface()
{
}


EELS_EEPROM_Interface::~EELS_EEPROM_Interface()
{
}
*/

uint32_t _eeprom_obj_write(uint16_t address, const void* buff, int32_t length){
    if (eels_eeprom::_eeprom_obj)
        return eels_eeprom::_eeprom_obj->eeprom_write(address, (const uint8_t*)buff, length);
    else
        return ~0u;
}

uint32_t _eeprom_obj_read(uint16_t address, void* buff, uint16_t length){
    if (eels_eeprom::_eeprom_obj)
        return eels_eeprom::_eeprom_obj->eeprom_read(address, (uint8_t*)buff, length);
    else
        return ~0u;
}
