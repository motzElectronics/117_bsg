#ifndef __TASK_WEB_EXCHANGE_H
#define __TASK_WEB_EXCHANGE_H

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/utils_pckgs_manager.h"
#include "../xDrvrs/Inc/simcom.h"
#include "../xDrvrs/Inc/spiflash.h"
#include "cmsis_os.h"
#include "main.h"

#define SESSION_OPENING     1
#define SESSION_AUTHORIZE   2
#define SESSION_SENDING     3
#define SESSION_CLOSING     4
#define SESSION_TCP_CLOSING 5

u8 openSendTcp(u8* data, u16 sz);
u8 fastSendTcp(u8 statSend);
u8 procReturnStatus(u8 ret);

#endif