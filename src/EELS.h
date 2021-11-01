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
typedef uint32_t    EELSAddr;
typedef uint16_t    EELSlotSize;
typedef uint16_t    EELSlotLen;
typedef uint8_t     EELSDataLen;



///////////////////////////////////////////////////////////////////////////////////////////////
//                          public functions
uint8_t EELS_Init();

uint8_t EELS_SetSlot    (EELSh slotNumber, EELSAddr begin_addr, EELSlotSize length, EELSDataLen data_length);

/// @param slotNumber
/// @param data
/// @return >= 0 - writen index
///         < 0  - error code
int     EELS_InsertLog  (EELSh slotNumber, const void* data);

bool    EELS_ReadLast   (EELSh slotNumber, void* const buf);

/// @param log_num >= 0 - index from head
///                < 0  - index from tail
bool    EELS_ReadIdx    (EELSh slotNumber, int log_num , void* const buf );

/// @param log_num >= 0 - index from tail
///                < 0  - index from head
#define EELS_ReadFromEnd( h, tail_idx , buf ) EELS_ReadIdx(h, -(tail_idx), buf)



///////////////////////////////////////////////////////////////////////////////////////////////
//                          private functions

typedef uint8_t     EELSEpoch;
typedef struct {
    EELSAddr    current_position;

    EELSAddr    begining;
    EELSlotSize length;

    EELSlotLen  current_counter;
    EELSlotLen  _counter_max;

    EELSEpoch epoch_counter;

    EELSDataLen slot_log_length;
    EELSDataLen raw_data_size;
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
bool     _EELS_ReadLog      (EELSh slotNumber, EELSAddr log_start_position, void* const buf);

uint8_t EELS_crc8(const void *data, EELSDataLen len);




///////////////////////////////////////////////////////////////////////////////////////////////
//                      dbg functions
EELSDataLen EELS_SlotLogSize(EELSh slotNumber);
EELSAddr    EELS_SlotBegin  (EELSh slotNumber);
EELSAddr    EELS_SlotEnd    (EELSh slotNumber);
uint8_t     EELS_ReadCell   (EELSAddr position);
void        EELS_WriteCell  (EELSAddr position, uint8_t val);



#endif
/* EELS_H_ */
