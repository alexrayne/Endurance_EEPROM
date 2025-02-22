#ifndef EELS_H_
#define EELS_H_

/*
 * EELS.h
 *
 * remastered: alexraynepe196@gmail.com
 * Created: 7/24/2019 12:10:27 PM
 *  Author: hasan.jaafar
 */



#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <EELS_Conf.h>



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

typedef int8_t      EELSHandle;
typedef EELSHandle  EELSh;

typedef uint32_t    EELSAddr;
typedef uint32_t    EELSlotSize;    // [bytes]
typedef uint16_t    EELSlotLen;     // [items count]
typedef uint8_t     EELSDataLen;
typedef uint8_t     EELSPageLen;

/// @brief >= 0 - index position in slot.
///         < 0 - error code
typedef int EELSIndex;



/// @brief eels records a headed with internal info.
///         So structures, that are headed with this EELS header, can writes faster since writes by one eeprom-operation.
///         @sa EELS_InsertRec
enum {
    EELS_RECSIZE_HEAD = 2,///< EELS_RECSIZE_HEAD
};

struct EELSRecHead {
    uint8_t    raw[EELS_RECSIZE_HEAD];
};
typedef struct EELSRecHead EELSRecHead;


#define EELS_RECORD_SIZE(t)     (sizeof(EELSRecHead) + sizeof(t))

///////////////////////////////////////////////////////////////////////////////////////////////

enum EELSErrorID{
#ifdef EELS_PT_DECL
    EELS_ERROR_WAIT         = 0,

#ifdef _EELS_ERROR_OK
    EELS_ERROR_OK           = _EELS_ERROR_OK,
#else
    EELS_ERROR_OK           = 1,
#endif

#else   // no pt-threads
    EELS_ERROR_OK           = 0,
#endif

    EELS_ERROR_NOK,

    EELS_ERROR_NOSLOT       = -1,
    // not supports if EELS_PageAlign try on pagesize < log record size
    EELS_ERROR_SMALLPAGE    = -2,
    EELS_ERROR_OUTOFSLOT    = -3,

};
typedef int EELSError;



///////////////////////////////////////////////////////////////////////////////////////////////

/// @brief provide proto-thread waiting test
#ifndef EELS_SCHEDULE
#ifdef EELS_PT_DECL
#define EELS_SCHEDULE(state)    ( (state) != EELS_ERROR_WAIT )
#else
#define EELS_SCHEDULE(state)    ((void)(state), true)
#endif
#endif



///////////////////////////////////////////////////////////////////////////////////////////////
//                          public functions

void    EELS_Init();


/// @brief provide aligning records on page boundary
/// @return EELS_ERROR_SMALLPAGE - if pagesize < log record size
/// @note resets page section to 0...page_size
EELSError EELS_PageAlign  (EELSh slotNumber, EELSPageLen page_size);

/// @brief provide room records in page section
EELSError EELS_PageSection(EELSh slotNumber, EELSPageLen page_offs, EELSPageLen sec_size);

EELSError EELS_SetSlot    (EELSh slotNumber, EELSAddr begin_addr, EELSlotSize length, EELSDataLen data_length);

/// @brief check that slot have inited, and can contain some records
EELSError EELS_SlotOnline(EELSh slotNumber);



//================================================================================================
/// @brief before start  EELS_InsertXXX, EELS_ReadXXX need wait that EELS slot not busy by concurent operation

#ifdef EELS_PT_DECL
bool EELS_Ready(EELSh slotNumber);
#else
/// if no proto-threads concurence used, no need waitings
#define EELS_Ready(...) true
#endif



EELSError EELS_CleanLog(EELSh slotNumber);

EELSError EELS_InsertLog  (EELSh slotNumber, const void* data);

/// @brief write eels-record content.
/// @arg   rec - slot record, with data content filled.
/// @description   Since record is contigous, it can writes to eeprom by single operation,
///         that provides write speed as fast as possible
EELSError EELS_InsertRec  (EELSh slotNumber, EELSRecHead* rec);



//================================================================================================
typedef EELSAddr    EELSPosition;

EELSPosition    EELS_PosIdx (EELSh slotNumber, int idx);
EELSPosition    EELS_PosNext(EELSh slotNumber, EELSPosition from);
unsigned        EELS_Len    (EELSh slotNumber);

EELSError       EELS_ReadPos(EELSh slotNumber, EELSPosition at , void* const buf );
EELSError       EELS_ReadIdx(EELSh slotNumber, unsigned rec_idx , void* const buf );

/// @param log_num >= 0 - index from head
///                < 0  - index from tail
EELSError       EELS_ReadFromHead    (EELSh slotNumber, int log_num , void* const buf );

/// @param log_num >= 0 - index from tail
///                < 0  - index from head
#define EELS_ReadFromEnd( h, tail_idx , buf ) EELS_ReadFromHead(h, -((int)(tail_idx)), buf)

EELSError       EELS_ReadLast   (EELSh slotNumber, void* const buf);



///////////////////////////////////////////////////////////////////////////////////////////////
//                          private functions

typedef uint8_t     EELSEpoch;
typedef struct {
    EELSPosition    write_position;

    EELSAddr    begining;
    EELSlotSize length;

    EELSlotLen  write_index;
    EELSlotLen  _counter_max;

    EELSDataLen slot_log_length;
    EELSDataLen raw_data_size;

    // page layout
#ifndef EELS_PAGE_SIZE
    EELSlotSize page_size;
    uint8_t     page_2pwr;
#endif
    uint8_t     page_records;
    EELSPageLen page_offs;
    EELSPageLen page_limit;

    // writing context
    EELSRecHead head;

    EELSEpoch epoch_counter;
    bool      overlaped;

#ifdef EELS_PT_DECL
    EELS_PT_DECL;
#endif

}EELSlot_t;

static inline
EELSlot_t*  EELSlot         (EELSh slotNumber){
    extern EELSlot_t EELslot_arr[];
    return EELslot_arr + slotNumber;
};

EELSPosition _EELS_FindLastPos  (EELSh slotNumber);
uint16_t _EELS_getHealthyLogs(EELSh slotNumber);
uint16_t _EELS_getHealthySequence(EELSh slotNumber);
EELSError   _EELS_ReadLog      (EELSh slotNumber, EELSAddr log_start_position, void* const buf);

uint8_t EELS_crc8(const void *data, EELSDataLen len);




///////////////////////////////////////////////////////////////////////////////////////////////
//                      dbg functions
EELSDataLen EELS_SlotLogSize(EELSh slotNumber);
EELSAddr    EELS_SlotBegin  (EELSh slotNumber);
EELSAddr    EELS_SlotEnd    (EELSh slotNumber);
uint8_t     EELS_ReadCell   (EELSAddr position);
void        EELS_WriteCell  (EELSAddr position, uint8_t val);

/// @brief check all active slots vs intersection
bool        EELS_validate_intersects(void);


#ifdef __cplusplus
}
#endif

#endif
/* EELS_H_ */
