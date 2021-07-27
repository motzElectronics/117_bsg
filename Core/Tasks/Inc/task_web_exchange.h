#ifndef __TASK_WEB_EXCHANGE_H
#define __TASK_WEB_EXCHANGE_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../xDrvrs/Inc/spiflash.h"
#include "../xDrvrs/Inc/simcom.h"
#include "../Utils/Inc/utils_pckgs_manager.h"


u8 openSendTcp(u8* data, u16 sz);
u8 fastSendTcp(u8 statSend);
u8 procReturnStatus(u8 ret);


#endif