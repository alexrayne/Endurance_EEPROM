#pragma once

#include <cstdint>

#ifndef TWI0
	#define TWI0 0
	#define I2CADDR 0


	typedef struct {
		uint16_t xx;
	} Twi;
#endif


#define E24LC64_WRITE_PAGE_SIZE 32 //byte
#define E24LC64_TOTAL_SIZE 8000 //byte
#define E24LC64_WRITE_DELAY_MS 5 //ms

#define E24LC64_ERROR 919


class E24LC64
{
	//variables
public:
protected:
private:
	uint8_t _I2Caddr;
	Twi* _pTWI;
	uint8_t* _myeepP;
	//functions
public:
	E24LC64(Twi* pTWI, uint8_t addr);
	~E24LC64();
	uint32_t eeprom_write(uint16_t address, const uint8_t* buff, int32_t length);
	uint32_t eeprom_write(uint16_t address, const uint8_t value);
	uint32_t eeprom_read(uint16_t address, uint8_t* buff, uint16_t length);
	uint32_t eeprom_read(uint16_t address, uint8_t* buff);
protected:
private:
	uint32_t _ByteWrite(uint16_t ROMADDR, const uint8_t val);
	uint32_t _PageWrite(uint16_t ROMADDR, const uint8_t *buff, uint8_t length);
	uint32_t _read(uint16_t address, uint8_t* buff, uint16_t length);

	//	E24LC64( const E24LC64 &c );
	//	E24LC64& operator=( const E24LC64 &c );

}; //E24LC64

