#ifndef __TASK_KEEP_ALIVE_H
#define __TASK_KEEP_ALIVE_H

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bsg.h"
#include "cmsis_os.h"
#include "main.h"

u16  getAdcVoltBat();
void pwrOffBsg();
void updRTC();
void generateMsgKeepAlive();
void generateMsgDevOff();
void generateMsgTabloFW();

ErrorStatus sendMsgTabloFW();
ErrorStatus sendMsgFWUpdated();
ErrorStatus sendMsgFWUpdateBegin();
ErrorStatus sendMsgDevOn();
ErrorStatus sendMsgDevOff();
ErrorStatus sendMsgStatistics();

#endif