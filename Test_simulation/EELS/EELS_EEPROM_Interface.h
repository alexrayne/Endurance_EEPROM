#pragma once


#define EELS_24LC64


#ifdef EELS_24LC64
	#include "E24LC64.h"
#else
	#error DEFINE SUPPORTED EEPROM CHIP
#endif

class eels_eeprom {
	public:
		~eels_eeprom();
	protected:
		void eels_eeprom_init(Twi* pTWI, uint8_t addr);
		#ifdef EELS_24LC64
			E24LC64* _eeprom_obj;
		#else
			void* _eeprom_obj; //JUST TO prevent compile time error..
		#endif
};


