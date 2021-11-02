#ifndef EELS_H_
#define EELS_H_

/*
 * EELS.h
 *
 * remastered: alexraynepe196@gmail.com
 * Created: 7/24/2019 12:10:27 PM
 *  Author: hasan.jaafar
 */



//#include "hal_flash.h"
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>



#ifdef __cplusplus
extern "C" {
#endif



//perhapbs this is not for configuration but to remind you about CRC byte:
//currently i only support CRC8
#define _EELS_CRC_BYTE_LENGTH 1

#ifdef EELS_PAGE_SIZE
// validate PAGE_SIZE is POW2
#if (EELS_PAGE_SIZE | (EELS_PAGE_SIZE-1)) != (2*EELS_PAGE_SIZE)-1
#error "supports only page_size= x^2"
#endif
#endif


///////////////////////////////////////////////////////////////////////////////////////////////

typedef uint8_t     EELSHandle;
typedef EELSHandle  EELSh;
typedef uint32_t    EELSAddr;
typedef uint16_t    EELSlotSize;
typedef uint16_t    EELSlotLen;
typedef uint8_t     EELSDataLen;
typedef uint8_t     EELSPageLen;



///////////////////////////////////////////////////////////////////////////////////////////////
//                          public functions
enum EELSErrorID{
    EELS_ERROR_OK           = 0,
    EELS_ERROR_NOSLOT       = -1,
    // not supports if EELS_PageAlign try on pagesize < log record size
    EELS_ERROR_SMALLPAGE    = -2,
};
typedef int EELSError;

void    EELS_Init();


/// @brief provide aligning records on page boundary
/// @return EELS_ERROR_SMALLPAGE - if pagesize < log record size
/// @note resets page section to 0...page_size
EELSError EELS_PageAlign  (EELSh slotNumber, EELSlotLen page_size);

/// @brief provide room records in page section
EELSError EELS_PageSection(EELSh slotNumber, EELSPageLen page_offs, EELSPageLen sec_size);

EELSError EELS_SetSlot    (EELSh slotNumber, EELSAddr begin_addr, EELSlotSize length, EELSDataLen data_length);



//================================================================================================

/// @brief >= 0 - index position in slot.
///         < 0 - error code
typedef int EELSIndex;

/// @param slotNumber
/// @param data
/// @return >= 0 - writen index
///         < 0  - error code
EELSIndex EELS_InsertLog  (EELSh slotNumber, const void* data);



//================================================================================================
typedef EELSAddr    EELSPosition;

EELSPosition    EELS_PosIdx (EELSh slotNumber, int idx);
EELSPosition    EELS_PosNext(EELSh slotNumber, EELSPosition from);
bool            EELS_ReadPos(EELSh slotNumber, EELSPosition at , void* const buf );

/// @param log_num >= 0 - index from head
///                < 0  - index from tail
bool    EELS_ReadIdx    (EELSh slotNumber, int log_num , void* const buf );

/// @param log_num >= 0 - index from tail
///                < 0  - index from head
#define EELS_ReadFromEnd( h, tail_idx , buf ) EELS_ReadIdx(h, -(tail_idx), buf)

bool    EELS_ReadLast   (EELSh slotNumber, void* const buf);



///////////////////////////////////////////////////////////////////////////////////////////////
//                          private functions

typedef uint8_t     EELSEpoch;
typedef struct {
    EELSPosition    write_position;

    EELSAddr    begining;
    EELSlotSize length;

    EELSlotLen  current_counter;
    EELSlotLen  _counter_max;

    EELSEpoch epoch_counter;

    EELSDataLen slot_log_length;
    EELSDataLen raw_data_size;

    // page layout
#ifndef EELS_PAGE_SIZE
    EELSlotSize page_size;
#endif
    EELSPageLen page_offs;
    EELSPageLen page_limit;
    uint8_t     page_records;

}EELSlot_t;

static inline
EELSlot_t*  EELSlot         (EELSh slotNumber){
    extern EELSlot_t EELslot_arr[];
    return EELslot_arr + slotNumber;
};

EELSAddr _EELS_FindLastPos  (EELSh slotNumber);
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



#ifdef __cplusplus
}
#endif

#endif
/* EELS_H_ */
