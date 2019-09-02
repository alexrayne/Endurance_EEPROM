
#include "stdio.h"
#include "unity.h"

#include "FlashMem.h"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#define SLOT_0 0





struct config_struct{
	uint8_t vt_timing_buffer[12]; //12 byte
	uint8_t buf[32-12];
}__attribute__((packed));



void EELS_init_test(){
	printf("========= init test ========== \n");
	struct config_struct last_cfg;
	EELS_Init(&ROM_Read,&ROM_Write);
	EELS_SetSlot(SLOT_0, (NVMCTRL_FLASH_SIZE - NVMCTRL_ROW_SIZE), NVMCTRL_ROW_SIZE,sizeof(struct config_struct));
	if (EELS_ReadLast(SLOT_0,(uint8_t*)&last_cfg)){
		printf("EELS_ReadLast: %d\n",EELS_ReadLast(SLOT_0,(uint8_t*)&last_cfg));
	}

	TEST_ASSERT_EQUAL_INT_MESSAGE(32 , sizeof(struct config_struct), "error in config struct size!!");
	TEST_ASSERT_TRUE_MESSAGE(  EELS_ReadLast(SLOT_0,(uint8_t*)&last_cfg), "last log read didnt match." );
	

}

void EELS_Insert_test(){
	printf("========= eels position test ========== \n");
	#define EELS_INSERT_COUNT 5000
	struct config_struct last_cfg;
	
	uint32_t last_pos = _EELS_FindLastPos(SLOT_0);
	uint32_t expected_pos = last_pos;
	last_cfg.vt_timing_buffer[0] = 0xef;
	clock_t begin = clock();
	for (int i=0 ; i<EELS_INSERT_COUNT; i++){
		if ((double)(clock() - begin) / CLOCKS_PER_SEC > 0.3 ||  i == EELS_INSERT_COUNT - 1 ){
			printf("\b%c[2K\r[%d/%d]", 27, i+1,EELS_INSERT_COUNT ); //progress bar
			fflush(stdout);
			begin = clock();
		}
		EELS_InsertLog(SLOT_0,(uint8_t*) &last_cfg);

		expected_pos += EELS_SlotLogSize(SLOT_0);
		if (expected_pos > EELS_SlotEnd(SLOT_0) - EELS_SlotLogSize(SLOT_0))
			expected_pos = EELS_SlotBegin(SLOT_0);

		TEST_ASSERT_EQUAL_INT_MESSAGE(expected_pos, _EELS_FindLastPos(SLOT_0), "expected position error");

	}
	printf("\n");
}

void EELS_readwrite_compare(){
	#define EELS_READWRITE_COUNT 10000
	/* initialize random seed: */
	srand (time(NULL));
		printf("========= eels read write compare ========== \n");
	struct config_struct last_cfg, read_cfg;
	clock_t begin = clock();
	for (int j=0; j<EELS_READWRITE_COUNT; j++){
		if ((double)(clock() - begin) / CLOCKS_PER_SEC > 0.3 || j == EELS_READWRITE_COUNT - 1 ){
			printf("\b%c[2K\r[%d/%d]", 27, j+1,EELS_READWRITE_COUNT ); //progress bar
			fflush(stdout);
			begin = clock();
		}


		for (int i=0; i< sizeof(last_cfg); i++){
			*(uint8_t*) &last_cfg = rand() % 256;
		}

		EELS_InsertLog(SLOT_0,(uint8_t*) &last_cfg);
		if (((uint32_t)rand() + rand() ) == 9876){ // possibility = EELS_READWRITE_COUNT/(32768*2) (RAND_MAX) 
			//_EELS_FindLastPos(SLOT_0,)
			*(uint8_t*) &last_cfg += 1;
		}
		
		EELS_ReadLast(SLOT_0, (uint8_t*)&read_cfg);
		int memcmpR = memcmp(&read_cfg, &last_cfg,  sizeof(last_cfg));
		if( memcmpR != 0){
			printf("error in readwritecompare [%d]", j);
			TEST_ASSERT_EQUAL_INT_MESSAGE(memcmpR, 0, "EELS_readwrite_compare");
		}
	}
	printf("\n");
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
	ROM_Commit();
    return UNITY_END();

}