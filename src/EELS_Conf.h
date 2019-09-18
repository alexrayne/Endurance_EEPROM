
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




#endif
/* EELS_CONF_H_ */
