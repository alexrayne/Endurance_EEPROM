#pragma once

#define EELS_24LC64


#ifdef __cplusplus

#ifdef EELS_24LC64
	#include "E24LC64.h"
#else
	#error DEFINE SUPPORTED EEPROM CHIP
#endif

extern E24LC64* _eeprom_obj;


class eels_eeprom {
	public:
		~eels_eeprom();

#ifdef EELS_24LC64
		static E24LC64* _eeprom_obj;
#else
		static void* _eeprom_obj; //JUST TO prevent compile time error..
#endif
	
	protected:
		void eels_eeprom_init(Twi* pTWI, uint8_t addr);
};


#endif // __cplus_plus

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
uint32_t _eeprom_obj_write(uint16_t address, const uint8_t* buff, int32_t length);
uint32_t _eeprom_obj_read(uint16_t address, uint8_t* buff, uint16_t length);

#ifdef __cplusplus
}
#endif
