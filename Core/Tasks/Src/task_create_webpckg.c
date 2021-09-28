#include "../Tasks/Inc/task_create_webpckg.h"

#include "../Tasks/Inc/task_iwdg.h"

extern u16 iwdgTaskReg;

extern osThreadId    webExchangeHandle;
extern osThreadId    keepAliveHandle;
extern osThreadId    getNewBinHandle;
extern osThreadId    createWebPckgHandle;
extern osMutexId     mutexWebHandle;
extern osMessageQId  queueWebPckgHandle;
extern osSemaphoreId semCreateWebPckgHandle;

extern u8   SZ_PCKGENERGY;
static char tmpBufPage[256];

static Page     pgEnergy = {.type = CMD_DATA_ENERGY_127, .szType = SZ_CMD_ENERGY_127};
static Page     pgTemp = {.type = CMD_DATA_TEMP, .szType = SZ_CMD_TEMP};
static Page     pgVoltAmper = {.type = CMD_DATA_VOLTAMPER_127, .szType = SZ_CMD_VOLTAMPER_127};
static Page     pgGrmc = {.type = CMD_DATA_GRMC, .szType = SZ_CMD_GRMC};
static Page     pgTelemetry = {.type = CMD_DATA_TELEMETRY, .szType = SZ_CMD_TELEMETRY};
static Page     pgPercRSSI = {.type = CMD_DATA_PERCRSSI_127, .szType = SZ_CMD_PERCRSSI_127};
static Page *   allPages[] = {&pgVoltAmper, &pgEnergy, &pgTemp, &pgGrmc, &pgTelemetry, &pgPercRSSI};
static WebPckg *curPckg;

void taskCreateWebPckg(void const *argument) {
    // vTaskSuspend(webExchangeHandle);

    u16 delayPages;
    u16 szAllPages = 0;
    u8  amntPages;
    u8  len;

    xSemaphoreTake(semCreateWebPckgHandle, 1);

    vTaskSuspend(createWebPckgHandle);

    LOG_WEB(LEVEL_MAIN, "taskCreateWebPckg\r\n");
    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_WEB_PCKG;
        delayPages = getDelayPages();
        while (delayPages > BSG_THRESHOLD_CNT_PAGES || (osSemaphoreWait(semCreateWebPckgHandle, 1) == osOK)) {
            curPckg = getFreePckg();
            if (curPckg == NULL) {
                break;
            }
            clearAllPages();
            if (bsg.csq < 10) {
                amntPages = delayPages > 3 ? 3 : delayPages;
            } else if (bsg.csq < 15) {
                amntPages = delayPages > 4 ? 4 : delayPages;
            } else if (bsg.csq <= 31) {
                amntPages = delayPages > 5 ? 5 : delayPages;
            } else {
                amntPages = delayPages > 3 ? 3 : delayPages;
            }
            // amntPages = delayPages > 3 ? 3 : delayPages;
            // amntPages = delayPages > AMOUNT_MAX_PAGES ? AMOUNT_MAX_PAGES : delayPages;
            for (u8 i = 0; i < amntPages; i++) {
                len = spiFlashReadLastPg((u8 *)tmpBufPage, 256, 0);
                if (len) {
                    parseData(tmpBufPage, len);
                }
            }
            szAllPages = getSzAllPages();
            if (szAllPages) {
                LOG_WEB(LEVEL_INFO, "Create package\r\n");
                initWebPckg(curPckg, szAllPages, 0, &bsg.idMCU);
                addPagesToWebPckg(curPckg);
                osMessagePut(queueWebPckgHandle, (u32)curPckg, osWaitForever);
            } else {
                freeWebPckg(curPckg);
            }
            delayPages = getDelayPages();
        }

        if (!delayPages) {
            LOG_WEB(LEVEL_DEBUG, "no pckg in spiflash\r\n");
            bsg.isSentData = 1;
        }
        osDelay(1000);
    }
}

void clearAllPages() {
    for (u8 i = 0; i < AMOUNT_MAX_PAGES; i++) {
        clearPage(allPages[i]);
    }
}

void parseData(u8 *tmpBufPage, u8 len) {
    u8 i = 0;
    while (i < len) {
        switch (tmpBufPage[i]) {
            case CMD_DATA_ENERGY_127:
                addToPage(&pgEnergy, &tmpBufPage[i + 1], SZ_CMD_ENERGY_127);
                i += (SZ_CMD_ENERGY_127 + 1);
                break;
            case CMD_DATA_VOLTAMPER_127:
                addToPage(&pgVoltAmper, &tmpBufPage[i + 1], SZ_CMD_VOLTAMPER_127);
                i += (SZ_CMD_VOLTAMPER_127 + 1);
                break;
            case CMD_DATA_TEMP:
                addToPage(&pgTemp, &tmpBufPage[i + 1], SZ_CMD_TEMP);
                i += (SZ_CMD_TEMP + 1);
                break;
            case CMD_DATA_TELEMETRY:
                addToPage(&pgTelemetry, &tmpBufPage[i + 1], SZ_CMD_TELEMETRY);
                i += (SZ_CMD_TELEMETRY + 1);
                break;
            case CMD_DATA_GRMC:
                addToPage(&pgGrmc, &tmpBufPage[i + 1], SZ_CMD_GRMC);
                i += (SZ_CMD_GRMC + 1);
                break;
            case CMD_DATA_PERCRSSI_127:
                addToPage(&pgPercRSSI, &tmpBufPage[i + 1], SZ_CMD_PERCRSSI_127);
                i += (SZ_CMD_PERCRSSI_127 + 1);
                break;
            default:
                LOG_WEB(LEVEL_ERROR, "ER: CMD_DATA_X is wrong\r\n");
                return;
                break;
        }
    }
}

void clearPage(Page *pg) {
    if (pg->iter) {
        pg->iter = 0;
        memset(pg->buf, SZ_PAGES, '\0');
    }
}

u16 getSzAllPages() {
    u16 sz = 0;
    for (u8 i = 0; i < AMOUNT_MAX_PAGES; i++) {
        if (allPages[i]->iter) {
            sz += (allPages[i]->iter + 1 + 1);  // sz + sizeof(type) + szeof(cnt)
        }
    }
    return sz;
}

void addToPage(Page *pg, u8 *src, u8 sz) {
    memcpy(&pg->buf[pg->iter], src, sz);
    pg->iter += sz;
}

void addPagesToWebPckg(WebPckg *pckg) {
    for (u8 i = 0; i < AMOUNT_MAX_PAGES; i++) {
        if (allPages[i]->iter) {
            addInfoToWebPckg(pckg, allPages[i]->buf, allPages[i]->iter, allPages[i]->iter / allPages[i]->szType, allPages[i]->type);
        }
    }
    closeWebPckg(pckg);
    // showWebPckg(pckg);
}