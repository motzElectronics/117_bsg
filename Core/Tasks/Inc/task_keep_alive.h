#ifndef __TASK_KEEP_ALIVE_H
#define __TASK_KEEP_ALIVE_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/circularBuffer.h"

u16 getAdcVoltBat();
void pwrOffBkte();
void updRTC();
void generateMsgKeepAlive();
void generateMsgDevOff();
#endif