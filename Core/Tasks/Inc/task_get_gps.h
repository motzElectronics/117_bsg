#ifndef __TASK_GET_GPS_H
#define __TASK_GET_GPS_H

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_gps.h"
#include "cmsis_os.h"
#include "main.h"

#define TRAIN_STOP 0
#define TRAIN_MOVE 1

void unLockTasks();
u8   checkStopTrain(PckgGnss* pckg);
void generateInitTelemetry();
void generateTestPackage();

#endif