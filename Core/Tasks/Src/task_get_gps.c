#include "../Tasks/Inc/task_get_gps.h"

#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_keep_alive.h"
#include "../Utils/Inc/utils_flash.h"
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
static u32      stopTime;
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

    // generateTestPackage();
    generateInitTelemetry();

    unLockTasks();

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_GPS;
        waitIdle("", &uInfoGnss.irqFlags, 1, 10000);
        if (uInfoGnss.irqFlags.isIrqIdle) {
            uInfoGnss.irqFlags.isIrqIdle = 0;
            // while (cBufRead(&circBufGnss, (u8 *)bufGnss, CIRC_TYPE_GNSS, 0)) {
            memcpy(bufGnss, uInfoGnss.pRxBuf, 200);
            // LOG_GPS(LEVEL_INFO, "GPS : %s", bufGnss);
            if (bsg.sleepTimer.flagOn) {
                memset(bufGnss, '\0', sizeof(bufGnss));
                break;
                // continue;
            }
            ret = fillGprmc(bufGnss, &pckgGnss);
            if (ret == GPS_OK) {
                updateCurCoords(&pckgGnss);
                if (checkStopTrain(&pckgGnss) == TRAIN_MOVE) {
                    if (!(numIteration % 2)) {
                        // LOG_GPS(LEVEL_INFO, "Move save\r\n");
                        serializePckgGnss(bufPckgGnss, &pckgGnss);
                        saveData((u8 *)&bufPckgGnss, SZ_CMD_GRMC, CMD_DATA_GRMC, &circBufAllPckgs);
                    }
                } else {
                    if (!numIteration) {
                        // LOG_GPS(LEVEL_INFO, "Stop save\r\n");
                        serializePckgGnss(bufPckgGnss, &pckgGnss);
                        saveData((u8 *)&bufPckgGnss, SZ_CMD_GRMC, CMD_DATA_GRMC, &circBufAllPckgs);
                    }
                }
                numIteration = (numIteration + 1) % 60;
            } else if (ret == GPS_GPRMC_ERR_INVALID_DATA_STATUS) {
                bsg.cur_gps.valid = 0;
                bsg.stat.gpsInvaligCount++;
            } else {
                bsg.stat.gpsParseFailCount++;
            }
            memset(bufGnss, '\0', sizeof(bufGnss));
            // }
            uartClearInfo(&uInfoGnss);
            __HAL_DMA_DISABLE(uInfoGnss.pHuart->hdmarx);
            __HAL_DMA_SET_COUNTER(uInfoGnss.pHuart->hdmarx, uInfoGnss.szRxBuf);
            __HAL_DMA_ENABLE(uInfoGnss.pHuart->hdmarx);
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

u8 checkStopTrain(PckgGnss *pckg) {
    static u8 cntr = 0;
    if (pckg->coords.speed < (5 * 10)) {
        stopTime++;
    } else {
        stopTime = 0;
    }

    if (stopTime > 10) {
        return TRAIN_STOP;
    }
    return TRAIN_MOVE;
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
    pckgTel.data = bsg.info.idBoot;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

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

    pckgTel.code = TEL_CD_HW_UPDATE_ERR;
    tmp = getFlashData(FLASH_ADDR_ERR_NEW_FIRMWARE);
    if (tmp > 10) tmp = 0;
    pckgTel.data = tmp;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    updSpiFlash(&circBufAllPckgs);
    xSemaphoreGive(semCreateWebPckgHandle);
}

void generateTestPackage() {
    PckgTelemetry pckgTel;
    PckgTemp      pckgTemp;

    LOG_WEB(LEVEL_MAIN, "Generate TEST package\r\n");

    pckgTel.group = 0x10;
    pckgTel.code = 0x01;
    pckgTel.data = 4;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    pckgTel.group = 0x10;
    pckgTel.code = 0x02;
    pckgTel.data = 3;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    pckgTel.group = 0x10;
    pckgTel.code = 0x03;
    pckgTel.data = 3;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    pckgTel.group = 0x10;
    pckgTel.code = 0x04;
    pckgTel.data = (3 << 16) | 7;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    pckgTel.group = 0x10;
    pckgTel.code = 0x05;
    pckgTel.data = (3 << 16) | 8;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    pckgTemp.unixTimeStamp = getUnixTimeStamp();
    pckgTemp.temp[0] = 102;
    pckgTemp.temp[1] = 0;
    pckgTemp.temp[2] = 3;
    pckgTemp.temp[3] = 16;
    saveData((u8 *)&pckgTemp, SZ_CMD_TEMP, CMD_DATA_TEMP, &circBufAllPckgs);
    pckgTemp.temp[2] = 4;
    pckgTemp.temp[3] = 17;
    saveData((u8 *)&pckgTemp, SZ_CMD_TEMP, CMD_DATA_TEMP, &circBufAllPckgs);
    pckgTemp.temp[2] = 5;
    pckgTemp.temp[3] = 16;
    saveData((u8 *)&pckgTemp, SZ_CMD_TEMP, CMD_DATA_TEMP, &circBufAllPckgs);
    pckgTemp.temp[2] = 6;
    pckgTemp.temp[3] = 15;
    saveData((u8 *)&pckgTemp, SZ_CMD_TEMP, CMD_DATA_TEMP, &circBufAllPckgs);

    updSpiFlash(&circBufAllPckgs);
    xSemaphoreGive(semCreateWebPckgHandle);
}
