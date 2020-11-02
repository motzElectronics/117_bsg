#ifndef __UTILS_CRC_H
#define __UTILS_CRC_H
#include "main.h"

u16 calcCrc16(unsigned char* pcBlock, unsigned short len);
u8 crc8(char *pcBlock, int len);
#endif