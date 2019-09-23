
#include <stdio.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include "unity.h"

#include "EELS.h"
#include "FlashMem.h"

#define CAT(a, b, c) PCAT(a, b, c)
#define PCAT(a,b,c) a##b##c


#define SLOT_0 0
#define SLOT_0_BEGIN (NVMCTRL_FLASH_SIZE - NVMCTRL_ROW_SIZE)
#define SLOT_0_LENGTH (NVMCTRL_ROW_SIZE)
#define SLOT_0_STRUCT struct config_struct
#define SLOT_0_DATALENGTH (sizeof(SLOT_0_STRUCT))

#define SLOT_1 1
#define SLOT_1_BEGIN (NVMCTRL_FLASH_SIZE - 10*NVMCTRL_ROW_SIZE)
#define SLOT_1_LENGTH (9*NVMCTRL_ROW_SIZE)
#define SLOT_1_STRUCT struct slot_1_wte
#define SLOT_1_DATALENGTH (sizeof(SLOT_1_STRUCT))



#define SLOT_UT SLOT_1
#define SLOT_UT_BEGIN CAT(SLOT_, SLOT_UT, _BEGIN )
#define SLOT_UT_LENGTH CAT(SLOT_, SLOT_UT, _LENGTH )
#define SLOT_UT_STRUCT CAT(SLOT_, SLOT_UT, _STRUCT)
#define SLOT_UT_DATALENGTH CAT(SLOT_, SLOT_UT, _DATALENGTH )


struct config_struct{
	uint8_t vt_timing_buffer[12]; //12 byte
	uint8_t buf[32-12];
}__attribute__((packed)); //32

struct slot_1_wte{
	uint8_t vt_timing_buffer[5];
	uint8_t buf[5];
}__attribute__((packed));



void EELS_init_test(){
	printf("\e[30m\e[31m========= init test ========== \e[0m\n");
	SLOT_UT_STRUCT last_cfg;
	//struct config_struct last_cfg;
	EELS_Init();
	EELS_SetSlot(SLOT_UT, SLOT_UT_BEGIN, SLOT_UT_LENGTH, SLOT_UT_DATALENGTH);
	//#define NVMCTRL_FLASH_SIZE          262144
	//NVMCTRL_ROW_SIZE            256
	if (EELS_ReadFromEnd(SLOT_UT,0,(uint8_t*)&last_cfg)){
		printf("EELS_Readfromend(0): %d\n",EELS_ReadFromEnd(SLOT_UT,0,(uint8_t*)&last_cfg));
	}

	TEST_ASSERT_EQUAL_INT (10 , SLOT_UT_DATALENGTH);
	TEST_ASSERT_TRUE_MESSAGE( EELS_ReadFromEnd(SLOT_UT,0,(uint8_t*)&last_cfg), "last log read didnt match." );

	printf("\tLOG_begin:0x%.8X\n",EELS_SlotBegin(SLOT_UT) );
	printf("\tLOG_size:%d\n",EELS_SlotLogSize(SLOT_UT) );
	printf("\tLOG_end:0x%.8X\n",EELS_SlotEnd(SLOT_UT) );
}

void EELS_Insert_test(){
	printf("\e[30m\e[93m========= eels position test ========== \e[0m\n");
	#define EELS_INSERT_COUNT 5000
	SLOT_UT_STRUCT last_cfg;

	uint32_t last_pos = _EELS_FindLastPos(SLOT_UT);
	uint32_t expected_pos = last_pos;
	last_cfg.vt_timing_buffer[0] = 0x0d;
	last_cfg.vt_timing_buffer[1] = 0xf0;
	last_cfg.vt_timing_buffer[2] = 0xbe;
	last_cfg.vt_timing_buffer[3] = 0xba;
	clock_t begin = clock();
	for (int i=0 ; i<EELS_INSERT_COUNT; i++){
		if ((double)(clock() - begin) / CLOCKS_PER_SEC > 0.3 ||  i == EELS_INSERT_COUNT - 1 ){
			printf("\b%c[2K\r[%d/%d]", 27, i+1, EELS_INSERT_COUNT ); //progress bar
			fflush(stdout);
			begin = clock();
		}
		EELS_InsertLog(SLOT_UT,(uint8_t*) &last_cfg);

		expected_pos += EELS_SlotLogSize(SLOT_UT);
		if (expected_pos > EELS_SlotEnd(SLOT_UT) - EELS_SlotLogSize(SLOT_UT))
			expected_pos = EELS_SlotBegin(SLOT_UT);

		TEST_ASSERT_EQUAL_INT(expected_pos, _EELS_FindLastPos(SLOT_UT));

	}
	printf("\n");
}




void EELS_readlogSeq_test(){
	printf("\e[30m\e[93m========= eels read sequences ========== \e[0m\n");
	uint16_t max_logs = (EELS_SlotEnd(SLOT_UT) - EELS_SlotBegin(SLOT_UT)) / EELS_SlotLogSize(SLOT_UT);
	printf("\t max_logs=%u \n",max_logs);
	SLOT_UT_STRUCT w_cfg, read_cfg;

	for (uint16_t j=0; j<max_logs; j++){
			memset(&w_cfg, j % 255 , SLOT_UT_DATALENGTH);
			EELS_InsertLog(SLOT_UT,(uint8_t*) &w_cfg);
	}

	for (uint16_t j=0; j<max_logs; j++){
		bool x = EELS_ReadFromEnd(SLOT_UT, max_logs - (j+1) , (uint8_t*)&read_cfg);
		TEST_ASSERT_EQUAL_INT( j % 255 , *(uint8_t*)&read_cfg);
	}

}


void EELS_readwrite_compare(){
	#define EELS_READWRITE_COUNT 10000
	/* initialize random seed: */
	srand (time(NULL));
	printf("\e[30m\e[93m========= eels read write compare ========== \e[0m\n");
	SLOT_UT_STRUCT last_cfg, read_cfg;
	clock_t begin = clock();
	for (int j=0; j<EELS_READWRITE_COUNT; j++){
		if ((double)(clock() - begin) / CLOCKS_PER_SEC > 0.3 || j == EELS_READWRITE_COUNT - 1 ){
			printf("\b\e[2K\r[%d/%d]", j+1,EELS_READWRITE_COUNT ); //progress bar
			fflush(stdout);
			begin = clock();
		}


		for (int i=0; i< sizeof(last_cfg); i++){
			*(uint8_t*) &last_cfg = rand() % 256;
		}

		EELS_InsertLog(SLOT_UT,(uint8_t*) &last_cfg);
		uint32_t rand_var = (uint32_t)rand() + rand();
		if (rand_var == 9876){ //------------------ possibility = EELS_READWRITE_COUNT/(32768*2) (RAND_MAX)
			//_EELS_FindLastPos(SLOT_UT,)
			*(uint8_t*) &last_cfg += 1;
		}

		EELS_ReadFromEnd(SLOT_UT,0, (uint8_t*)&read_cfg);
		int memcmpR = memcmp(&read_cfg, &last_cfg,  sizeof(last_cfg));
		if( memcmpR != 0 && rand_var != 9876){
			printf("error in readwritecompare [%d]\n", j);
			printf("\t last_cfg: 0x[%X]\n", *(uint32_t*) &last_cfg);
			printf("\t read_cfg 0x[%X]\n", *(uint32_t*) &read_cfg);
			TEST_ASSERT_EQUAL_INT(memcmpR, 0 );
		}
	}
	printf("\n");
}

void EELS_LogHealth_test(){
	printf("\e[30m\e[93m========= eels healthy logs ========== \e[0m\n");
	uint32_t Start_pos = EELS_SlotBegin(SLOT_UT);
	uint32_t log_size = EELS_SlotLogSize(SLOT_UT);
	uint32_t End_pos = EELS_SlotEnd(SLOT_UT);

	uint16_t expected_healthy = floor((End_pos - Start_pos) / log_size);
	TEST_ASSERT_EQUAL_INT_MESSAGE(_EELS_getHealthyLogs(SLOT_UT), expected_healthy, "\e[91mEELS_logHealth_test\e[0m");


	for (uint16_t i= expected_healthy; i>0; i--){
		//printf("healthy logs:%d\texpected:%d\n", _EELS_getHealthyLogs(SLOT_UT),i);
		uint32_t mypos = Start_pos + (log_size*i);
		TEST_ASSERT_EQUAL_INT_MESSAGE(_EELS_getHealthyLogs(SLOT_UT), i, "healthy logs CRC didnt match expected...");
		EELS_WriteCell( mypos -1 , EELS_ReadCell(mypos -1) - 1);
	}
	/*fix the curropted:*/
	for (uint32_t i= Start_pos; i<End_pos; i+=log_size){
		if (i+log_size > End_pos)
				continue;
		uint32_t mypos =i+log_size;
		EELS_WriteCell( mypos -1 , EELS_ReadCell(mypos -1) + 1);
	}

	TEST_ASSERT_EQUAL_INT_MESSAGE(expected_healthy, _EELS_getHealthyLogs(SLOT_UT), "\e[91mEELS_logHealth_test\e[0m");
	printf("sequences: %d \n",_EELS_getHealthySequence(SLOT_UT));
	TEST_ASSERT_EQUAL_INT(2, _EELS_getHealthySequence(SLOT_UT));
}




void setUp(void) {
    // set stuff up here

}

void tearDown(void) {
    // clean stuff up here
}


int main(){
	UNITY_BEGIN();
	ROM_Init();
		RUN_TEST(EELS_init_test);
		RUN_TEST(EELS_Insert_test);
		RUN_TEST(EELS_readwrite_compare);
		RUN_TEST(EELS_LogHealth_test);
		RUN_TEST(EELS_readlogSeq_test);
		//printf("slotB:%d \t slotE:%d \t slotLL:%d \t Healthy:%d\n", EELS_SlotBegin(SLOT_0) , EELS_SlotEnd(SLOT_0) , EELS_SlotLogSize(SLOT_0) , _EELS_getHealthyLogs(SLOT_0));
	ROM_Commit();
	//getchar();
	UNITY_END();

    return 0;
}
