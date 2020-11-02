#ifndef __TASK_HTTP_H
#define __TASK_HTTP_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_gps.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../xDrvrs/Inc/simcom.h"
#include "../Utils/Inc/utils_json.h"

#define PCKG_WAS_lOST   1
#define PCKG_WAS_SENT   0

u8 isDataFromFlashOk(char* pData, u8 len);
u8 getReadyTxPages();

u8 sendDataToServer();
void saveCsq(u8 csq);

#endif