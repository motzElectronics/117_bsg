#include "../Tasks/Inc/task_get_gps.h"

#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_keep_alive.h"
#include "tim.h"

extern u16 iwdgTaskReg;

extern osMutexId mutexGPSBufHandle;

extern osThreadId webExchangeHandle;
extern osThreadId keepAliveHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId getNewBinHandle;
extern osThreadId getTrainDataHandle;

extern osSemaphoreId semCreateWebPckgHandle;

static char     bufGnss[200];
static PckgGnss pckgGnss;
static u8       bufPckgGnss[SZ_CMD_GRMC];
CircularBuffer  circBufGnss = {.buf = NULL, .max = 0};

extern CircularBuffer rxUart1CircBuf;
extern CircularBuffer circBufAllPckgs;

void taskGetGPS(void const *argument) {
    u8 ret;
    u8 numIteration = 0;
    cBufInit(&circBufGnss, uInfoGnss.pRxBuf, uInfoGnss.szRxBuf, CIRC_TYPE_GNSS);

    spiFlashInit(circBufAllPckgs.buf);
    cBufReset(&circBufAllPckgs);

    simInit();
    getServerTime();

    generateInitTelemetry();
    unLockTasks();

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_GPS;
        waitRx("", &uInfoGnss.irqFlags, 10, 10000);
        if (uInfoGnss.irqFlags.isIrqRx) {
            uInfoGnss.irqFlags.isIrqRx = 0;
            while (cBufRead(&circBufGnss, (u8 *)bufGnss, CIRC_TYPE_GNSS, 0)) {
                LOG_GPS(LEVEL_DEBUG, "GPS : %s", bufGnss);
                if (bsg.sleepTimer.flagOn) {
                    memset(bufGnss, '\0', sizeof(bufGnss));
                    continue;
                }
                ret = fillGprmc(bufGnss, &pckgGnss);
                if (ret == GPS_OK) {
                    updateCurCoords(&pckgGnss);
                    if (!numIteration) {
                        serializePckgGnss(bufPckgGnss, &pckgGnss);
                        saveData((u8 *)&bufPckgGnss, SZ_CMD_GRMC, CMD_DATA_GRMC, &circBufAllPckgs);
                    }
                    numIteration = (numIteration + 1) % 120;
                } else if (ret == GPS_GPRMC_ERR_INVALID_DATA_STATUS) {
                    bsg.cur_gps.valid = 0;
                    bsg.stat.gpsInvaligCount++;
                } else {
                    bsg.stat.gpsParseFailCount++;
                }
                memset(bufGnss, '\0', sizeof(bufGnss));
            }
        } else {
            LOG_GPS(LEVEL_ERROR, "ERROR: NOT WORKING GPS\r\n");
        }
    }
    /* USER CODE END taskGetGPS */
}

void unLockTasks() {
    LOG(LEVEL_MAIN, "unLockTasks\r\n");
    vTaskResume(webExchangeHandle);
    vTaskResume(createWebPckgHandle);
    vTaskResume(keepAliveHandle);
    vTaskResume(getTrainDataHandle);
}

void checkStopTrain(PckgGnss *pckg) {
    static u8 cntr = 0;
    if (pckg->coords.speed < (5 * 10)) {
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
    long long     phoneNum;
    u32           tmp;
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
