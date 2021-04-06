#include "../Tasks/Inc/task_web_exchange.h"

extern osThreadId webExchangeHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;

static WebPckg* curPckg = NULL;

void taskWebExchange(void const* argument) {
    u8 statSend = TCP_OK;

    vTaskSuspend(webExchangeHandle);

    for (;;) {
        if (bsg.isTCPOpen == 0) {
            osMutexWait(mutexWebHandle, osWaitForever);
            openTcp();
            osMutexRelease(mutexWebHandle);
        } else if (bsg.isTCPOpen == 1) {
            // if (statSend == TCP_OK || statSend == TCP_SEND_ER_LOST_PCKG) {
            if (statSend == TCP_OK) {
                if (curPckg != NULL) {
                    clearWebPckg(curPckg);
                    curPckg = NULL;
                }
                xQueueReceive(queueWebPckgHandle, &curPckg, portMAX_DELAY);
            }

            osMutexWait(mutexWebHandle, osWaitForever);
            statSend = sendTcp(curPckg->buf, curPckg->shift);
            osMutexRelease(mutexWebHandle);

            osDelay(10);
        }
    }
}
