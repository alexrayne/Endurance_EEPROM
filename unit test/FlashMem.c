/*
 * FlashMem.c
 *
 * Created: 7/24/2019 3:40:32 PM
 *  Author: hasan.jaafar
 */ 


#include "FlashMem.h"
#include <stdio.h>

//#include "App.h"


 

uint8_t flashMem[FSIZE];

int32_t ROM_Init(){
	FILE * pFile;
	size_t result;
	pFile = fopen ( ".\\eeprom.bin" , "rb" );
	if (pFile==NULL){
		fputs ("File error , writing new file\n",stderr);
		FILE *f = fopen("eeprom.bin", "wb");
		fwrite(flashMem, sizeof(uint8_t), sizeof(flashMem), f);
		fclose(f);
		return -1;
	}
		
	result = fread (flashMem,1, FSIZE * sizeof(uint8_t),pFile);
	fclose (pFile);
	if (result != FSIZE){
		fputs ("read error\n",stderr);
		printf("Result: %d \t FSIZE: %d \n",result, FSIZE);
		return -1;
	}
}

int32_t ROM_Read(uint32_t addr, uint8_t* const buf, uint16_t length){
	//return flash_read(&FLASH_0, addr, buf, length);
	memcpy(buf,flashMem+addr,length);
	return 0;
}

int32_t ROM_Write(uint32_t addr, uint8_t* const buf, uint16_t length){
	//return flash_write(&FLASH_0, addr,buf,length);
	memcpy(flashMem+addr,buf,length);
	return 0;
}

int32_t ROM_Commit(){
	FILE *f = fopen(".\\eeprom.bin", "wb");
	fwrite(flashMem, sizeof(uint8_t), sizeof(flashMem), f);
	fclose(f);
	return 0;
}

