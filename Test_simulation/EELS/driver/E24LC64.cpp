/*
* E24LC64.cpp
*
* Created: 04/2/2019 9:21:33 AM
* Author: hasan.jaafar
*/

#include <stdafx.h>

#include "E24LC64.h"

// default constructor
E24LC64::E24LC64(Twi* pTWI, uint8_t addr) {
	this->_I2Caddr = addr;
	this->_pTWI = pTWI;
	_myeepP = new uint8_t[E24LC64_TOTAL_SIZE];
	printf("_myeepP created\r\n");
}


/* this function to write on the EEPROM, taking care of page boundary and page write. */
/*there is 5ms delay in the last page write.. what if the user will not write data.. why should we delay him 5ms?*/
uint32_t E24LC64::eeprom_write(uint16_t _address, const uint8_t* buff, int32_t length) {
	//length variable :	signed int because i need to detect that length is positive 
	uint8_t sent_length;
	//uint32_t TwiErrCode = TWI_SUCCESS;
	//do we need to write next page:
	while ((length>0)) {
		_PageWrite(_address, buff, length);
		//calculate the sent length (again?):
		sent_length = E24LC64_WRITE_PAGE_SIZE - (_address % E24LC64_WRITE_PAGE_SIZE);
		//increase the address , decrease the length, increase the buffer pointer
		length -= sent_length;
		_address += sent_length;
		buff += sent_length;
	}
	return 0;
}
//write single byte..
uint32_t E24LC64::eeprom_write(uint16_t address, const uint8_t value) {
	return _ByteWrite(address, value);
}


//read sequential ...
uint32_t E24LC64::eeprom_read(uint16_t address, uint8_t* buff, uint16_t length) {
	if ((length + address > E24LC64_TOTAL_SIZE) || (address >= E24LC64_TOTAL_SIZE))
		return E24LC64_ERROR;

	return _read(address, buff, length);
}
//read single byte
uint32_t E24LC64::eeprom_read(uint16_t address, uint8_t* buff) {
	if (address >= E24LC64_TOTAL_SIZE)
		return E24LC64_ERROR;
	return _read(address, buff, 1);
}

//========================================================================================================================
//private functions:

uint32_t E24LC64::_read(uint16_t address, uint8_t* buff, uint16_t length) {
	memcpy(buff,this->_myeepP+address, length);
	/*twi_packet_t packet;
	packet.buffer = buff;
	packet.length = length;
	packet.chip = _I2Caddr;
	packet.addr[0] = uint8_t((address) >> 8); //high byte
	packet.addr[1] = uint8_t(address); //low byte
	packet.addr_length = 2;
	uint32_t err_val = twi_master_read(_pTWI, &packet);
	return err_val;*/
	return 0;
}

uint32_t E24LC64::_ByteWrite(uint16_t ROMADDR, const uint8_t val) {
	_myeepP[ROMADDR] = val;
	/*twi_packet_t packet;
	//char data[] = {address,value};
	packet.buffer = (void*)&val;
	packet.length = 1;
	packet.chip = _I2Caddr;
	packet.addr[0] = uint8_t((ROMADDR) >> 8); //high byte
	packet.addr[1] = uint8_t(ROMADDR); //low byte
	packet.addr_length = 2;
	uint32_t err_val = twi_master_write(_pTWI, &packet);
	if (err_val == TWI_SUCCESS)
		delay_ms(E24LC64_WRITE_DELAY_MS);
	return err_val;*/
	return 0;
}

/*
this function write pages
it trims the right bytes if they exceed the page boundary.
max length = EEPROM page size.
*/
uint32_t E24LC64::_PageWrite(uint16_t ROMADDR, const uint8_t *buff, uint8_t length) {
	uint8_t send_length;

	uint8_t page_head_addr = (ROMADDR % E24LC64_WRITE_PAGE_SIZE);

	if ((page_head_addr + length)>E24LC64_WRITE_PAGE_SIZE) {
		send_length = E24LC64_WRITE_PAGE_SIZE - page_head_addr; //trim the extra
	}
	else {
		send_length = length;
	}

	memcpy(_myeepP+ROMADDR, buff, send_length);
	/*
	twi_packet_t packet;
	packet.buffer = (void*)buff;
	packet.length = send_length;
	packet.chip = _I2Caddr;
	packet.addr[0] = uint8_t((ROMADDR) >> 8); //high byte
	packet.addr[1] = uint8_t(ROMADDR); //low byte
	packet.addr_length = 2;
	uint32_t TwiErrCode = twi_master_write(_pTWI, &packet);

	if (TwiErrCode == TWI_SUCCESS)
		delay_ms(E24LC64_WRITE_DELAY_MS);
	return TwiErrCode;*/

	return 0;
}


// default destructor
E24LC64::~E24LC64()
{
	delete[] _myeepP;
}
