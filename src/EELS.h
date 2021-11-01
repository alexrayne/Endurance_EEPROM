#ifndef EELS_H_
#define EELS_H_

/*
 * EELS.h
 *
 * Created: 7/24/2019 12:10:27 PM
 *  Author: hasan.jaafar
 */



//#include "hal_flash.h"
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>



//perhapbs this is not for configuration but to remind you about CRC byte:
//currently i only support CRC8
#define _EELS_CRC_BYTE_LENGTH 1



typedef uint8_t     EELSHandle;
typedef EELSHandle  EELSh;




///////////////////////////////////////////////////////////////////////////////////////////////
//                          public functions
uint8_t EELS_Init();
void    EELS_InsertLog  (EELSh slotNumber, const void* data);
uint8_t EELS_SetSlot    (EELSh slotNumber, uint32_t begin_addr, uint16_t length, uint8_t data_length);
bool    EELS_ReadFromEnd(EELSh slotNumber, int log_num , void* const buf );
bool    EELS_ReadLast   (EELSh slotNumber, void* const buf);



///////////////////////////////////////////////////////////////////////////////////////////////
//                          private functions

typedef uint8_t     EELSEpoch;
typedef struct {
    uint32_t current_position;

    uint32_t begining;
    uint16_t length;

    uint16_t current_counter;
    uint16_t _counter_max;

    EELSEpoch epoch_counter;

    uint8_t slot_log_length;
    uint8_t raw_data_size;
    uint8_t page_records;

}EELSlot_t;

static inline
EELSlot_t*  EELSlot         (EELSh slotNumber){
    extern EELSlot_t EELslot_arr[];
    return EELslot_arr + slotNumber;
};

uint32_t _EELS_FindLastPos  (EELSh slotNumber);
uint16_t _EELS_getHealthyLogs(EELSh slotNumber);
uint16_t _EELS_getHealthySequence(EELSh slotNumber);
bool     _EELS_ReadLog      (EELSh slotNumber, uint32_t log_start_position, void* const buf);

uint8_t EELS_crc8(const void *data, uint8_t len);




///////////////////////////////////////////////////////////////////////////////////////////////
//                      dbg functions
uint8_t     EELS_SlotLogSize(EELSh slotNumber);
uint32_t    EELS_SlotBegin  (EELSh slotNumber);
uint32_t    EELS_SlotEnd    (EELSh slotNumber);
uint8_t     EELS_ReadCell   (uint32_t position);
void        EELS_WriteCell  (uint32_t position, uint8_t val);



#endif
/* EELS_H_ */
