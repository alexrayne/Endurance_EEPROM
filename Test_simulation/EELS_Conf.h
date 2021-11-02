
#ifndef EELS_CONF_H_
#define EELS_CONF_H_



/* Include the eeprom functions file here: */
#include "EELS/EELS_EEPROM_Interface.h"

/* DEFINE EEPROM FUNCTIONS HERE: */
#define EELS_EEPROM_READ(addr,buf,length)   _eeprom_obj_read(addr, buf, length)
#define EELS_EEPROM_WRITE(addr,buf,length)  _eeprom_obj_write(addr,buf,length)

/* how many slots are used in your application: */
#define EELS_APP_SLOTS 5

/* DEBUG */
#define __EELS_DBG__ (0)



/*CRC8*/
#define EELS_LOCAL_CRC_FN (1)
#if EELS_LOCAL_CRC_FN
  #define EELS_CRC8(...) EELS_crc8(__VA_ARGS__)
#else
  /*crc_function(uint8_t *data, uint8_t len)*/
  #define EELS_CRC8(...) YOUR_CRC_FUNCTION(__VA_ARGS__)

#endif


#endif
/* EELS_CONF_H_ */
