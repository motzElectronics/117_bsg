#include "../Tasks/Inc/task_create_webpckg.h"

extern osThreadId    webExchangeHandle;
extern osThreadId    keepAliveHandle;
extern osThreadId    getNewBinHandle;
extern osThreadId    createWebPckgHandle;
extern osMutexId     mutexWebHandle;
extern osMessageQId  queueWebPckgHandle;
extern osSemaphoreId semCreateWebPckgHandle;

extern u8   SZ_PCKGENERGY;
static char tmpBufPage[256];

static Page     pgEnergy = {.type = CMD_DATA_ENERGY, .szType = SZ_CMD_ENERGY};
static Page     pgTemp = {.type = CMD_DATA_TEMP, .szType = SZ_CMD_TEMP};
static Page     pgVoltAmper = {.type = CMD_DATA_VOLTAMPER, .szType = SZ_CMD_VOLTAMPER};
static Page     pgGrmc = {.type = CMD_DATA_GRMC, .szType = SZ_CMD_GRMC};
static Page     pgTelemetry = {.type = CMD_DATA_TELEMETRY, .szType = SZ_CMD_TELEMETRY};
static Page *   allPages[] = {&pgVoltAmper, &pgEnergy, &pgTemp, &pgGrmc, &pgTelemetry};
static WebPckg *curPckg;

void taskCreateWebPckg(void const *argument) {
    // vTaskSuspend(webExchangeHandle);

    s16 delayPages;
    u16 szAllPages = 0;
    u8  amntPages;
    u8  len;

    xSemaphoreTake(semCreateWebPckgHandle, 1);

    vTaskSuspend(createWebPckgHandle);

    D(printf("taskCreateWebPckg\r\n"));
    for (;;) {
        delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? spiFlash64.headNumPg - spiFlash64.tailNumPg : spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
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
                spiFlashRdPg((u8 *)tmpBufPage, 256, 0, spiFlash64.tailNumPg);

                if ((len = isDataFromFlashOk(tmpBufPage, 256))) {
                    parceData(tmpBufPage, len);
                } else {
                    D(printf("ERROR: bad crc isDataFromFlashOk()\r\n"));
                }
            }
            szAllPages = getSzAllPages();
            initWebPckg(curPckg, szAllPages, 0);
            addPagesToWebPckg(curPckg);
            osMessagePut(queueWebPckgHandle, (u32)curPckg, osWaitForever);
            delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? spiFlash64.headNumPg - spiFlash64.tailNumPg : spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
        }

        if (!delayPages) {
            // D(printf("no pckg in spiflash\r\n"));
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

void parceData(u8 *tmpBufPage, u8 len) {
    u8 i = 0;
    while (i < len) {
        switch (tmpBufPage[i]) {
            case CMD_DATA_ENERGY:
                addToPage(&pgEnergy, &tmpBufPage[i + 1], SZ_CMD_ENERGY);
                i += (SZ_CMD_ENERGY + 1);
                break;
            case CMD_DATA_VOLTAMPER:
                addToPage(&pgVoltAmper, &tmpBufPage[i + 1], SZ_CMD_VOLTAMPER);
                i += (SZ_CMD_VOLTAMPER + 1);
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
            default:
                D(printf("ER: CMD_DATA_X is wrong\r\n"));
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
    showWebPckg(pckg);
}