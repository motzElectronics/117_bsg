/*
 * utils_flash.h
 *
 *  Created on: 5 мая 2020 г.
 *      Author: annsi
 */

#ifndef INC_UTILS_FLASH_H_
#define INC_UTILS_FLASH_H_
#include "../Utils/Inc/utils_bsg.h"
#include "main.h"
#define FLASH_ADDR_ID_BOOT          0x08004000
#define FLASH_ADDR_IS_NEW_FIRWARE   0x08004004
#define FLASH_ADDR_SZ_NEW_FIRMWARE  0x08004008
#define FLASH_ADDR_START            0x08000000
#define FLASH_ADDR_FIRMWARE         0x08008000
#define FLASH_EMPTY_SECTOR          0xFFFFFFFF

#define FLASH_ADDR_BUF_NEW_FIRMWARE 0x8040000

void flashClearPage(u32 page);
void flashWrite(u8* pData, u32 szHex, u32* distAddr);

#endif /* INC_UTILS_FLASH_H_ */
