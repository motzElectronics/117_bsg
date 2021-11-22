#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_web_exchange.h"

extern u16 iwdgTaskReg;

extern osThreadId    webExchangeHandle;
extern osThreadId    createWebPckgHandle;
extern osThreadId    createWebPckgHandle;
extern osMutexId     mutexWebHandle;
extern osMessageQId  queueWebPckgHandle;
extern osSemaphoreId semSendWebPckgHandle;

static WebPckg* curPckg = NULL;

ErrorStatus sendAuthorizePckg();
ErrorStatus sendEndSessionPckg();

void taskWebExchange(void const* argument) {
    u8  statSend = TCP_OK;
    u8  sessionStep = SESSION_OPENING;
    u32 order_num = 0;

    osSemaphoreWait(semSendWebPckgHandle, 1);

    vTaskSuspend(webExchangeHandle);

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_WEB_EXCH;

        if (!bsg.isTCPOpen) {
            sessionStep = SESSION_OPENING;
        }

        switch (sessionStep) {
            case SESSION_OPENING:
                if (bsg.isTCPOpen) {
                    sessionStep = SESSION_AUTHORIZE;
                    continue;
                }
                if (osSemaphoreWait(semSendWebPckgHandle, 30000) != osOK) {
                    continue;
                }
                // LOG_WEB(LEVEL_INFO, "FLUSH CONTINUED 2\r\n");
                osMutexWait(mutexWebHandle, osWaitForever);
                openTcp(bsg.server);
                osMutexRelease(mutexWebHandle);
                break;
            case SESSION_AUTHORIZE:
                if (bsg.server == SERVER_MOTZ) {
                    sessionStep = SESSION_SENDING;
                    continue;
                }
                if (sendAuthorizePckg() == SUCCESS) {
                    sessionStep = SESSION_SENDING;
                } else {
                    osDelay(1000);
                }
                break;
            case SESSION_SENDING:
                if (statSend == TCP_OK) {
                    if (curPckg != NULL) {
                        clearWebPckg(curPckg);
                        curPckg = NULL;
                    }
                    osEvent evt = osMessageGet(queueWebPckgHandle, 500);
                    if (evt.status == osEventMessage) {
                        curPckg = (WebPckg*)evt.value.p;
                    } else {
                        sessionStep = SESSION_CLOSING;
                        continue;
                    }
                }

                osMutexWait(mutexWebHandle, osWaitForever);
                if (bsg.server == SERVER_MOTZ) {
                    memcpy(&order_num, &curPckg->buf[2], 4);
                } else {
                    memcpy(&order_num, &curPckg->buf[1], 4);
                }
                // LOG_WEB(LEVEL_INFO, "TCP Send: sz %d, num %d, addr 0x%08x\r\n", curPckg->shift, order_num, curPckg);
                LOG_WEB(LEVEL_INFO, "TCP Send: sz %d, num %d\r\n", curPckg->shift, order_num);
                statSend = sendTcp(bsg.server, curPckg->buf, curPckg->shift);
                if (statSend == TCP_OK) {
                    clearWebPckg(curPckg);
                    curPckg = NULL;
                }
                osMutexRelease(mutexWebHandle);
                break;
            case SESSION_CLOSING:
                if (bsg.server == SERVER_MOTZ) {
                    sessionStep = SESSION_TCP_CLOSING;
                    continue;
                }
                if (sendEndSessionPckg() == SUCCESS) {
                    sessionStep = SESSION_TCP_CLOSING;
                } else {
                    osDelay(1000);
                }
                break;
            case SESSION_TCP_CLOSING:
                if (!bsg.isTCPOpen) {
                    sessionStep = SESSION_OPENING;
                    continue;
                }
                osMutexWait(mutexWebHandle, osWaitForever);
                closeTcp();
                osMutexRelease(mutexWebHandle);
                break;

            default:
                sessionStep = SESSION_TCP_CLOSING;
                break;
        }
    }
}

ErrorStatus sendAuthorizePckg() {
    u8          statSend = TCP_OK;
    ErrorStatus ret = SUCCESS;
    WebPckg*    pckg;

    pckg = getFreePckgReq();
    if (pckg == NULL) {
        LOG_WEB(LEVEL_ERROR, "NO FREE PCKG\r\n");
        return ERROR;
    }

    makeAuthorizePckg(pckg, bsg.server);

    osMutexWait(mutexWebHandle, osWaitForever);
    LOG_WEB(LEVEL_INFO, "TCP Send: authorize\r\n");
    statSend = sendTcp(bsg.server, pckg->buf, pckg->shift);
    if (statSend != TCP_OK) ret = ERROR;

    clearWebPckg(pckg);
    pckg = NULL;

    osMutexRelease(mutexWebHandle);

    return ret;
}

ErrorStatus sendEndSessionPckg() {
    u8          statSend = TCP_OK;
    ErrorStatus ret = SUCCESS;
    WebPckg*    pckg;

    pckg = getFreePckgReq();
    if (pckg == NULL) {
        LOG_WEB(LEVEL_ERROR, "NO FREE PCKG\r\n");
        return ERROR;
    }

    makeEndSessionPckg(pckg, bsg.server);

    osMutexWait(mutexWebHandle, osWaitForever);
    LOG_WEB(LEVEL_INFO, "TCP Send: end session\r\n");
    statSend = sendTcp(bsg.server, pckg->buf, pckg->shift);
    if (statSend != TCP_OK) ret = ERROR;

    clearWebPckg(pckg);
    pckg = NULL;

    osMutexRelease(mutexWebHandle);

    return ret;
}
