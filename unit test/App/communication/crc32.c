/*
 * CFile1.c
 *
 * Created: 17/07/2018 11:42:22 AM
 *  Author: hasan.jaafar
 */ 
#include "crc32.h"

uint32_t xcrc32 (const char *buf, int len)
{
	uint32_t crc = 0xff1fff1fu;
	while (len--)
	{
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}

uint8_t xcrc8(uint8_t *data, uint8_t len)
{
	uint8_t crc = 0xff;
	uint8_t i, j;
	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if ((crc & 0x80) != 0)
			crc = (uint8_t)((crc << 1) ^ 0x31);
			else
			crc <<= 1;
		}
	}
	return crc;
}