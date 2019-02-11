#include "stdafx.h"
#include "EELS_EEPROM_Interface.h"


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