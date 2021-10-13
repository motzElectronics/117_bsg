#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_web_exchange.h"

extern u16 iwdgTaskReg;

extern osThreadId   webExchangeHandle;
extern osThreadId   createWebPckgHandle;
extern osThreadId   createWebPckgHandle;
extern osMutexId    mutexWebHandle;
extern osMessageQId queueWebPckgHandle;

static WebPckg* curPckg = NULL;

void taskWebExchange(void const* argument) {
    u8  statSend = TCP_OK;
    u32 order_num = 0;

    vTaskSuspend(webExchangeHandle);

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_WEB_EXCH;
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
                osEvent evt = osMessageGet(queueWebPckgHandle, osWaitForever);
                if (evt.status == osEventMessage) {
                    curPckg = (WebPckg*)evt.value.p;
                } else {
                    continue;
                }
            }

            osMutexWait(mutexWebHandle, osWaitForever);
            memcpy(&order_num, &curPckg->buf[2], 4);
            // LOG_WEB(LEVEL_INFO, "TCP Send: sz %d, num %d, addr 0x%08x\r\n", curPckg->shift, order_num, curPckg);
            LOG_WEB(LEVEL_INFO, "TCP Send: sz %d, num %d\r\n", curPckg->shift, order_num);
            statSend = sendTcp(curPckg->buf, curPckg->shift);
            if (statSend == TCP_OK) {
                clearWebPckg(curPckg);
                curPckg = NULL;
            }
            osMutexRelease(mutexWebHandle);

            osDelay(10);
        }
    }
}
