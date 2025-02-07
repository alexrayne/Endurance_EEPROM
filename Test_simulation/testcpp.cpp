// testcpp.cpp : Defines the entry point for the console application.
//


//#include "stdafx.h"


#include <memory>
#include <cstdio>
#include <iostream>
#include <windows.h>

#include "EELS.hpp"


#define EELS_S0_data_length 5
#define EELS_S1_data_length 8

#define EELS_S31_data_length 5
#define EELS_S32_data_length 8

#define EELS_S3_page_size    32



void PrintSlot(EELS* LogSys, uint8_t SlotNumber, uint8_t linelength);
void DumpSlot(EELS* LogSys, uint8_t SlotNumber);
void CheckLast(EELS* LogSys, uint8_t SlotNumber);

uint8_t meh[50];

int main()
{

	//E24LC64 mymem(TWI0, I2CADDR);
	EELS LogSys(TWI0, I2CADDR, 2);
	LogSys.SetSlot(0, 0, 100, EELS_S0_data_length);
	LogSys.SetSlot(1, 100, 150, EELS_S1_data_length);

	// paged slot
	LogSys.PageAlign(2, 16);
    LogSys.SetSlot(2, 250, 50, EELS_S0_data_length);

    // sections in same pages slots
    LogSys.PageAlign(3, EELS_S3_page_size);
    LogSys.PageSection(3, 0, 15);
    LogSys.PageAlign(4, EELS_S3_page_size);
    LogSys.PageSection(4, 16, 16);

	LogSys.SetSlot(3, 300, 150, EELS_S0_data_length);
    LogSys.SetSlot(4, 300, 150, EELS_S1_data_length);

	/*
	uint8_t addrzero[] = { 0,0,0,0 };
	mymem.eeprom_write(0, addrzero, 4);
	*/
	LogSys.WriteEeprom(0, 0);
	LogSys.WriteEeprom(1, 0);
	LogSys.WriteEeprom(2, 0);
	LogSys.WriteEeprom(49, 0);
	LogSys.WriteEeprom(100, 0x00);

	memset(meh, 0xff, 50);

	char cinchar = '+';
	while (cinchar != '~')
	{
		if (cinchar == '+') {
			LogSys.InsertLog(0, meh); 
			LogSys.InsertLog(1, meh); 
			LogSys.InsertLog(2, meh);
			LogSys.InsertLog(3, meh);
			LogSys.InsertLog(4, meh);

			CheckLast(&LogSys, 0);
			CheckLast(&LogSys, 1);
			CheckLast(&LogSys, 2);
			CheckLast(&LogSys, 3);
			CheckLast(&LogSys, 4);

			if (meh[0] == 0xff)
				memset(meh, 0xee, 50);
			else
				memset(meh, 0xff, 50);

		}
		else if (cinchar == '-') {
			LogSys.CleanLog(0);
			LogSys.CleanLog(1);
			LogSys.CleanLog(2);
			LogSys.CleanLog(3);
			LogSys.CleanLog(4);
		}
		else {
			std::cin >> cinchar;
			continue;
		}

        DumpSlot(&LogSys, 0);
        DumpSlot(&LogSys, 1);
        DumpSlot(&LogSys, 2);
        DumpSlot(&LogSys, 3);
        DumpSlot(&LogSys, 4);

		std::cin >> cinchar;
	} ;
	

	std::cin.get();
    return 0;
}

void DumpSlot(EELS* LogSys, uint8_t h){
    unsigned linelen = LogSys->PageSize(h);
    if (linelen <= 0)
		linelen = LogSys->GetLogSize(h)*2;
    PrintSlot(LogSys, h, linelen);
    printf("Slot(%u) last position:%d[%u]\r\n", (unsigned)h
                    , LogSys->GetLastPos(h), LogSys->GetCounter(h)
                    );
}


void PrintSlot(EELS* LogSys, uint8_t SlotNumber, uint8_t linelength){
	
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	uint16_t S_length = (*LogSys)[SlotNumber].length;
	uint16_t S_begin = (*LogSys)[SlotNumber].begining;

	for (uint16_t i = S_begin; i <S_begin+S_length; i++)
	{
		if ( (i- S_begin) % linelength == 0)
			printf("\r\n%6d\t\t", i);
		if (i == LogSys->GetLogPos(SlotNumber))
			SetConsoleTextAttribute(hConsole, 0x20);
		else
			SetConsoleTextAttribute(hConsole, 7);
		printf("%02x ", LogSys->ReadEeprom(i));
	}
	SetConsoleTextAttribute(hConsole, 7);
	printf("\r\n");

}

void CheckLast(EELS* LogSys, uint8_t SlotNumber) {
	uint8_t buf[50];

	EELSError ok = LogSys->rec_last(SlotNumber, buf);
	if (ok != EELS_ERROR_OK)
		printf("fails read last slot[%d] ok %d\n", (int)SlotNumber, (int)ok);
}
