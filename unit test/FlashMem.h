/*
 * FlashMem.h
 *
 * Created: 7/24/2019 10:55:30 AM
 *  Author: hasan.jaafar
 */


#ifndef FLASHMEM_H_
#define FLASHMEM_H_

#include <stdint.h>
#include <string.h>

#define FLASHMEM_LOCKREGIONS 16
#define FLASHMEM_REGIONSIZE (NVMCTRL_FLASH_SIZE / FLASHMEM_LOCKREGIONS)

#define FLASHMEM_LASTREGION_ADDR (NVMCTRL_FLASH_SIZE - FLASHMEM_REGIONSIZE)


#define NVMCTRL_FLASH_SIZE          262144
#define NVMCTRL_LOCKBIT_ADDRESS     0x00802000
#define NVMCTRL_PAGE_HW             32
#define NVMCTRL_PAGE_SIZE           64
#define NVMCTRL_PAGE_W              16
#define NVMCTRL_PMSB                3
#define NVMCTRL_PSZ_BITS            6
#define NVMCTRL_ROW_PAGES           4
#define NVMCTRL_ROW_SIZE            256



int32_t ROM_Read(uint32_t addr, uint8_t* const buf, uint16_t length);
int32_t ROM_Write(uint32_t addr, uint8_t* const buf, uint16_t length);

int32_t ROM_Init();
int32_t ROM_Commit();

#endif /* FLASHMEM_H_ */
