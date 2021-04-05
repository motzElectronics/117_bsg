#include "../Tasks/Inc/task_keep_alive.h"

extern osThreadId keepAliveHandle;
extern u8 isRxNewFirmware;

extern CircularBuffer circBufAllPckgs;

void taskKeepAlive(void const * argument){
    u16 timeout = 1;
    vTaskSuspend(keepAliveHandle);
    
    for(;;)
    {
        if(!(timeout % 60) && !isRxNewFirmware){
            D(printf("\r\ngetNumFirmware\r\n\r\n"));
            getNumFirmware();
        }
        if(!(timeout % 600) && !isRxNewFirmware){
            D(printf("\r\ngenerateMsgKeepAlive\r\n\r\n"));
            generateMsgKeepAlive();
        }
        if(!(timeout % 5400) && !isRxNewFirmware){
            D(printf("\r\nupdRTC\r\n\r\n"));
            updRTC();
        }
        
        timeout++;
        osDelay(2000);
    }
}




void updRTC(){

    getServerTime();
}

void generateMsgKeepAlive(){
    PckgTelemetry pckgTel;
	pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BSG_ALIVE;
	pckgTel.data = 0;
	saveTelemetry(&pckgTel, &circBufAllPckgs);
}