#ifndef __TASK_GET_GPS_H
#define __TASK_GET_GPS_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_gps.h"

void unLockTasks();
void checkStopTrain(PckgGnss* pckg);
void generateInitTelemetry();

#endif