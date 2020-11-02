#include "../Tasks/Inc/task_keep_alive.h"

extern osThreadId keepAliveHandle;
extern osThreadId getGPSHandle;
extern u8 isRxNewFirmware;

extern osMutexId mutexWebHandle;

extern HttpUrl urls;

void taskKeepAlive(void const * argument){
    u16 timeout = 1;
    vTaskSuspend(keepAliveHandle);

    for(;;)
    {
        if(!(timeout % 600) && !isRxNewFirmware){
            generateMsgKeepAlive();
        }
        if(!(timeout % 5400) && !isRxNewFirmware){
            updRTC();
        }
        timeout++;
        osDelay(2000);
    }
}


void updRTC(){
    u8 csq = 0;
    time_t prevRtcTime;
    u32 time = 0;
    xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
    while((csq = simCheckCSQ()) < 12 && csq > 99){
        osDelay(2000);
    }
    prevRtcTime = getTimeStamp();
    time = getServerTime();

    if(time - prevRtcTime > BSG_BIG_DIF_RTC_SERVTIME){
        fillTelemetry(TEL_BIG_DIFFER_RTC_SERVERTIME, time - prevRtcTime);   
    }

    fillTelemetry(TEL_CHANGE_TIME, time);

    simHttpInit(urls.addMeasure);
    xSemaphoreGive(mutexWebHandle);
}

void generateMsgKeepAlive(){
    fillTelemetry(TEL_KEEP_ALIVE, 0);
}