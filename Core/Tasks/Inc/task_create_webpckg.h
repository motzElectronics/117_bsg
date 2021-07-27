#ifndef __TASK_CREATE_WEB_PCKG_H
#define __TASK_CREATE_WEB_PCKG_H

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/utils_pckgs_manager.h"
#include "../xDrvrs/Inc/simcom.h"
#include "../xDrvrs/Inc/spiflash.h"
#include "cmsis_os.h"
#include "main.h"

#define PCKG_WAS_lOST 1
#define PCKG_WAS_SENT 0

typedef struct {
    u16 iter;
    u8  type;
    u8  szType;
    u8  buf[SZ_PAGES];
} Page;

u8   sendDataToServer();
void saveCsq(u8 csq);

void clearPage(Page* pg);
void clearAllPages();
void parceData(u8* tmpBufPage, u8 len);
u16  getSzAllPages();
void addToPage(Page* pg, u8* src, u8 sz);
void addPagesToWebPckg();

#endif