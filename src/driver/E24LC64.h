/* 
* E24LC64.h
*
* Created: 04/2/2019 9:21:34 AM
* Author: hasan.jaafar
*/


#ifndef __E24LC64_H__
#define __E24LC64_H__

#include <compiler.h>
// From module: TWI - Two-Wire Interface - SAM implementation
#include <sam_twi/twi_master.h>
#include <sam_twi/twi_slave.h>
#include <twi_master.h>
#include <twi_slave.h>

// From module: TWI - Two-wire Interface
#include <twi.h>

// From module: Delay routines
#include <delay.h>


#define E24LC64_WRITE_PAGE_SIZE 32
#define E24LC64_TOTAL_SIZE 8000
#define E24LC64_WRITE_DELAY_MS 5 



class E24LC64
{
//variables
public:
protected:
private:
	uint8_t _I2Caddr;
	Twi* _pTWI;
	
//functions
public:
	E24LC64(Twi* pTWI, uint8_t addr);
	~E24LC64();
	uint32_t eeprom_write(uint16_t address, const uint8_t* buff,int32_t length);
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

#endif //__E24LC64_H__
