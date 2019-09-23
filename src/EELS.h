/*
 * EELS.h
 *
 * Created: 7/24/2019 12:10:27 PM
 *  Author: hasan.jaafar
 */



//#include "hal_flash.h"
#include <limits.h>
#include <stdint.h>

/*
	page_size = flash_get_page_size(&FLASH_0);

	for (i = 0; i < page_size; i++) {
		src_data[i] = i;
	}

	flash_write(&FLASH_0, 0x3200, src_data, page_size);

	flash_read(&FLASH_0, 0x3200, chk_data, page_size);

*/ //.
#ifndef EELS_H_
#define EELS_H_

//perhapbs this is not for configuration but to remind you about CRC byte:
//currently i only support CRC8
#define _EELS_CRC_BYTE_LENGTH 1

typedef struct {
	uint32_t begining;
	uint16_t length;
	uint32_t current_position;
	uint16_t current_counter;
	uint8_t slot_log_length;
	uint8_t raw_data_size;
	uint16_t _counter_max;
	uint8_t _counter_bytes;
}ee_slot_t;


// public functions
uint8_t EELS_Init();
void EELS_InsertLog(uint8_t slotNumber, uint8_t* data);
uint8_t EELS_SetSlot(uint8_t slotNumber, uint32_t begin_addr, uint16_t length, uint8_t data_length);
bool EELS_ReadFromEnd(uint8_t slotNumber, uint16_t log_num , uint8_t* const buf );
bool EELS_ReadLast(uint8_t slotNumber, uint8_t* const buf);
//private functions
uint32_t _EELS_FindLastPos(uint8_t slotNumber);
uint16_t _EELS_getHealthyLogs(uint8_t slotNumber);
uint16_t _EELS_getHealthySequence(uint8_t slotNumber);
bool _EELS_ReadLog(uint8_t slotNumber, uint32_t log_start_position, uint8_t* const buf);

// dbg functions
uint8_t EELS_SlotLogSize(uint8_t slotNumber);
uint32_t EELS_SlotBegin(uint8_t slotNumber);
uint32_t EELS_SlotEnd(uint8_t slotNumber);
uint8_t EELS_ReadCell(uint32_t position);
void EELS_WriteCell(uint32_t position, uint8_t val);

#endif
/* EELS_H_ */
