
#ifndef EELS_CONF_H_
#define EELS_CONF_H_

/* Conf of EELS - EELogSlots, a messages ring buffer in EE system.
 * */


/* how many slots are used in your application: */
#define EELS_APP_SLOTS 20

/* DEBUG */
#define __EELS_DBG__ (0)



/*CRC8*/
#define EELS_CRC8(...) EELS_crc8(__VA_ARGS__)





/* DEFINE EEPROM FUNCTIONS HERE: */
#define EELS_EEPROM_READ(addr,buf,length)   ROM_Read(addr,buf,length)
#define EELS_EEPROM_WRITE(addr,buf,length)  ROM_Write(addr,buf,length)


///////////////////////////////////////////////////////////////////////////////
///     proto-threads API

#if 0

// contiki  proto-threads api
#include <sys/pt.h>

#define EELS_PT_DECL    struct pt pt

#endif







#endif
/* EELS_CONF_H_ */
