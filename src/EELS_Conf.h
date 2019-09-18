
#ifndef EELS_CONF_H_
#define EELS_CONF_H_



/* Include the eeprom functions file here: */
#include "FlashMem.h"
/* DEFINE EEPROM FUNCTIONS HERE: */
#define EELS_EEPROM_READ(addr,buf,length) ROM_Read(addr,buf,length)
#define EELS_EEPROM_WRITE(addr,buf,length) ROM_Write(addr,buf,length)

/* how many slots are used in your application: */
#define EELS_APP_SLOTS 5

/* DEBUG */
#define __EELS_DBG__ (0)



/*CRC8*/
#define EELS_LOCAL_CRC_FN (1)
#if EELS_LOCAL_CRC_FN
  uint8_t EELS_crc8(uint8_t *data, uint8_t len);
  #define EELS_CRC8(...) EELS_crc8(__VA_ARGS__)
#else
  /*crc_function(uint8_t *data, uint8_t len)*/
  #define EELS_CRC8(...) YOUR_CRC_FUNCTION(__VA_ARGS__)

#endif


#endif
/* EELS_CONF_H_ */
