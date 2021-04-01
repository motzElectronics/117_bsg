#include "../Tasks/Inc/task_get_gps.h"

#include "tim.h"
extern osMutexId mutexGPSBufHandle;

extern osThreadId webExchangeHandle;
extern osThreadId keepAliveHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId getNewBinHandle;

extern osSemaphoreId semCreateWebPckgHandle;

static char bufGnss[200];
static PckgGnss pckgGnss;
static u8 bufPckgGnss[SZ_CMD_GRMC];
CircularBuffer circBufGnss = {.buf = NULL, .max = 0};

extern CircularBuffer rxUart1CircBuf;
extern CircularBuffer circBufAllPckgs;

void taskGetGPS(void const *argument) {
    cBufInit(&circBufGnss, uInfoGnss.pRxBuf, uInfoGnss.szRxBuf, CIRC_TYPE_GNSS);

    spiFlashInit(circBufAllPckgs.buf);
    cBufReset(&circBufAllPckgs);

    simInit();
    while (openTcp() != TCP_OK);
    getServerTime();

    generateInitTelemetry();
    unLockTasks();

    for (;;) {
        waitRx("", &uInfoGnss.irqFlags, 1000, 10000);
        if (uInfoGnss.irqFlags.isIrqRx) {
            uInfoGnss.irqFlags.isIrqRx = 0;
            while (cBufRead(&circBufGnss, (u8 *)bufGnss, CIRC_TYPE_GNSS, 0)) {
                // D(printf("GPS: %s", bufGnss));
                if (!bsg.sleepTimer.flagOn &&
                    fillGprmc(bufGnss, &pckgGnss) == GPS_OK) {
                    // checkStopTrain(&pckgGnss);
                    serializePckgGnss(bufPckgGnss, &pckgGnss);
                    saveData((u8 *)&bufPckgGnss, SZ_CMD_GRMC, CMD_DATA_GRMC, &circBufAllPckgs);
                }
                memset(bufGnss, '\0', sizeof(bufGnss));
            }
            // D(printf("empty circBufGnss\r\n"));
        } else {
            D(printf("ERROR: NOT WORKING GPS\r\n"));
        }
    }
    /* USER CODE END taskGetGPS */
}

void unLockTasks() {
    D(printf("unLockTasks\r\n"));
    vTaskResume(webExchangeHandle);
    vTaskResume(createWebPckgHandle);
    vTaskResume(keepAliveHandle);
}

void checkStopTrain(PckgGnss *pckg) {
    static u8 cntr = 0;
    if (pckg->speed < (5 * 10)) {
        cntr++;
        if (cntr == 5) {
            cntr = 0;
            bsg.sleepTimer.flagOn = 1;
            HAL_TIM_Base_Start_IT(&htim10);
        }
    } else {
        cntr = 0;
        bsg.sleepTimer.flagOn = 0;
    }
}

void generateInitTelemetry() {
    PckgTelemetry pckgTel;
    long long phoneNum;
    u32 tmp;
    pckgTel.group = TEL_GR_GENINF;
    pckgTel.code = TEL_CD_GENINF_NUM_FIRMWARE;
    pckgTel.data = BSG_ID_FIRMWARE;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    pckgTel.code = TEL_CD_GENINF_NUM_BOOT;
    pckgTel.data = BSG_ID_BOOT;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    // pckgTel.code = TEL_CD_GENINF_NUM_PCB;
    // pckgTel.data = BSG_ID_PCB;
    // saveTelemetry(&pckgTel, &circBufAllPckgs);

    phoneNum = simGetPhoneNum();
    if (phoneNum > 0) {
        tmp = phoneNum % 1000000000;
        pckgTel.code = TEL_CD_GENINF_PHONE_NUM1;
        pckgTel.data = tmp;
        saveTelemetry(&pckgTel, &circBufAllPckgs);
    }

    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BSG;
    pckgTel.data = 1;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    updSpiFlash(&circBufAllPckgs);
    xSemaphoreGive(semCreateWebPckgHandle);
}
