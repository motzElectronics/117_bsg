#include "../Tasks/Inc/task_send_telemetry.h"

#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_web_exchange.h"

extern u16 iwdgTaskReg;

extern osThreadId    sendTelemetryHandle;
extern osMutexId     mutexWebHandle;
extern osMutexId     mutexSessionHandle;
extern osMessageQId  queueTelPckgHandle;
extern osSemaphoreId semSendTelPckgHandle;

static WebPckg* curPckg = NULL;

void taskSendTelemetry(void const* argument) {
    u8  statSend = TCP_OK;
    u8  sessionStep = SESSION_OPENING;
    u32 order_num = 0;

    osSemaphoreWait(semSendTelPckgHandle, 1);

    vTaskSuspend(sendTelemetryHandle);

    for (;;) {
#if SEND_TEL_TO_MOTZ
        iwdgTaskReg |= IWDG_TASK_REG_SEND_TEL;

        // if (bsg.isTCPOpen != SERVER_MOTZ) {
        //     sessionStep = SESSION_OPENING;
        // }

        switch (sessionStep) {
            case SESSION_OPENING:
                if (bsg.isTCPOpen == SERVER_MOTZ) {
                    sessionStep = SESSION_SENDING;
                    continue;
                }
                if (osSemaphoreWait(semSendTelPckgHandle, 180000) != osOK) {
                    continue;
                }
                osMutexWait(mutexSessionHandle, osWaitForever);
                LOG_WEB(LEVEL_INFO, "{--- Start sending TEL\r\n");

                osMutexWait(mutexWebHandle, osWaitForever);
                if (openTcp(SERVER_MOTZ) != TCP_OK) {
                    osMutexRelease(mutexSessionHandle);
                }
                osMutexRelease(mutexWebHandle);
                break;
            case SESSION_SENDING:
                if (statSend == TCP_OK) {
                    if (curPckg != NULL) {
                        clearWebPckg(curPckg);
                        curPckg = NULL;
                    }
                    osEvent evt = osMessageGet(queueTelPckgHandle, 500);
                    if (evt.status == osEventMessage) {
                        curPckg = (WebPckg*)evt.value.p;
                    } else {
                        sessionStep = SESSION_TCP_CLOSING;
                        continue;
                    }
                }

                osMutexWait(mutexWebHandle, osWaitForever);
                memcpy(&order_num, &curPckg->buf[2], 4);
                u32 ttt = HAL_GetTick();
                statSend = sendTcp(SERVER_MOTZ, curPckg->buf, curPckg->shift);
                ttt = HAL_GetTick() - ttt;
                if (statSend == TCP_OK) {
                    LOG_WEB(LEVEL_INFO, "TCP Send: num %d, sz %d, time %d\r\n", order_num, curPckg->shift, ttt);
                    clearWebPckg(curPckg);
                    curPckg = NULL;
                } else {
                    sessionStep = SESSION_TCP_CLOSING;
                    LOG_WEB(LEVEL_ERROR, "TCP Send: num %d, sz %d, time %d Failed\r\n", order_num, curPckg->shift, ttt);
                }
                osMutexRelease(mutexWebHandle);
                break;
            case SESSION_TCP_CLOSING:
                if (!bsg.isTCPOpen) {
                    sessionStep = SESSION_OPENING;
                    osMutexRelease(mutexSessionHandle);
                    continue;
                }
                osMutexWait(mutexWebHandle, osWaitForever);
                closeTcp();
                osMutexRelease(mutexWebHandle);

                osMutexRelease(mutexSessionHandle);
                LOG_WEB(LEVEL_INFO, "---} Finish sending TEL\r\n");
                osDelay(60000);
                break;

            default:
                sessionStep = SESSION_TCP_CLOSING;
                break;
        }
#endif
        osDelay(100000);
    }
}
