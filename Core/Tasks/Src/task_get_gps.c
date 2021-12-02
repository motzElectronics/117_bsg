#include "../Tasks/Inc/task_get_gps.h"

#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_keep_alive.h"
#include "../Utils/Inc/utils_flash.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"

extern u16 iwdgTaskReg;

extern osMutexId mutexGPSBufHandle;
extern osMutexId mutexRTCHandle;
extern osMutexId mutexSessionHandle;

extern osThreadId webExchangeHandle;
extern osThreadId keepAliveHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId getNewBinHandle;
extern osThreadId getTrainDataHandle;
extern osThreadId sendTelemetryHandle;

extern osSemaphoreId semCreateWebPckgHandle;

static char     bufGnss[300];
static PckgGnss pckgGnss;
static u8       bufPckgGnss[SZ_CMD_GEO_PLUS];
CircularBuffer  circBufGnss = {.buf = NULL, .max = 0};

extern CircularBuffer rxUart1CircBuf;
extern CircularBuffer circBufAllPckgs;

static RTC_TimeTypeDef stmTime;
static RTC_DateTypeDef stmDate;

u8 setGPSUnixTime(DateTime *dt);

void taskGetGPS(void const *argument) {
    u8  ret;
    u8  gpsParceErrCnt = 0;
    u16 gpsParseErr = 0;
    u32 numIteration = 0;
    u8  gps_step = GPS_STEP_NONE;

    PckgTelemetry pckgTel;

    cBufInit(&circBufGnss, uInfoGnss.pRxBuf, uInfoGnss.szRxBuf, CIRC_TYPE_GNSS);

    spiFlashInit(circBufAllPckgs.buf);
    cBufReset(&circBufAllPckgs);

    while (bsg.imei < 10000000000) {
        simInit();
        bsg.imei = simGetIMEI();
    }

    // TODO: time should be syncronized by GPS
    getServerTime();
    generateInitTelemetry();
    unLockTasks();
    gps_step = GPS_STEP_TIMESYNC;

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_GPS;
        waitIdle("", &uInfoGnss.irqFlags, 1, 10000);
        if (uInfoGnss.irqFlags.isIrqIdle) {
            uInfoGnss.irqFlags.isIrqIdle = 0;
            memcpy(bufGnss, uInfoGnss.pRxBuf, 300);
            // LOG_GPS(LEVEL_INFO, "\r\n%s", bufGnss);
            ret = fillGprmc(bufGnss, &pckgGnss);
            switch (gps_step) {
                case GPS_STEP_NONE:
                    gps_step = GPS_STEP_TIMESYNC;
                    break;
                case GPS_STEP_TIMESYNC:
                    if (ret == GPS_OK) {
                        if (!setGPSUnixTime(&pckgGnss.dateTime)) {
                            break;
                        }
                        // generateInitTelemetry();
                        // unLockTasks();

                        gps_step = GPS_STEP_WORK;
                    }
                    break;
                case GPS_STEP_WORK:
                    if (ret == GPS_OK) {
                        updateCurCoords(&pckgGnss);
                        if (checkStopTrain(&pckgGnss) == TRAIN_MOVE) {
                            // LOG_GPS(LEVEL_INFO, "Move save\r\n");
                            serializePckgGnss(bufPckgGnss, &pckgGnss);
                            saveData((u8 *)&bufPckgGnss, SZ_CMD_GEO_PLUS, CMD_DATA_GEO_PLUS, &circBufAllPckgs);
                        } else {
                            if (!(numIteration % 30)) {
                                // LOG_GPS(LEVEL_INFO, "Stop save\r\n");
                                serializePckgGnss(bufPckgGnss, &pckgGnss);
                                saveData((u8 *)&bufPckgGnss, SZ_CMD_GEO_PLUS, CMD_DATA_GEO_PLUS, &circBufAllPckgs);
                            }
                        }
                        if (!(numIteration % 1800)) {
                            setGPSUnixTime(&pckgGnss.dateTime);
                        }
                        numIteration++;
                        gpsParceErrCnt = 0;
                        gpsParseErr = 0;
                    } else if (ret == GPS_GPRMC_ERR_INVALID_DATA_STATUS) {
                        bsg.cur_gps.valid = 0;
                        gpsParceErrCnt = 0;
                        gpsParseErr = 0;
                        bsg.stat.gpsInvaligCount++;
                    } else {
                        bsg.cur_gps.valid = 0;
                        bsg.stat.gpsParseFailCount++;
                        gpsParceErrCnt++;
                        gpsParseErr |= (1 << ret);
                        if (gpsParceErrCnt == 15) {
                            pckgTel.group = TEL_GR_SIMCOM;
                            pckgTel.code = TEL_CD_SIM_GPS_PARSE_ERROR;
                            pckgTel.data = gpsParseErr;
                            saveTelemetry(&pckgTel, &circBufAllPckgs);
                            gpsParceErrCnt = 0;
                            gpsParseErr = 0;
                            bsg.cur_gps.valid = 0;

                            osMutexWait(mutexSessionHandle, osWaitForever);
                            simReset();
                            osMutexRelease(mutexSessionHandle);
                        }
                    }
                    break;
                default:
                    gps_step = GPS_STEP_NONE;
                    break;
            }
            memset(bufGnss, '\0', sizeof(bufGnss));
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
    // vTaskResume(sendTelemetryHandle);
}

u8 checkStopTrain(PckgGnss *pckg) {
    if (pckg->coords.speed < (5 * 10)) {
        bsg.cur_gps.stopTime++;
    } else {
        bsg.cur_gps.stopTime = 0;
    }

    if (bsg.cur_gps.stopTime > 10) {
        return TRAIN_STOP;
    }
    return TRAIN_MOVE;
}

u8 setGPSUnixTime(DateTime *dt) {
    u8 ret = 0;
    if (dt != NULL) {
        stmTime.Hours = dt->hour;
        stmTime.Minutes = dt->min;
        stmTime.Seconds = dt->sec;

        stmDate.Date = dt->day;
        stmDate.Month = dt->month;
        stmDate.Year = dt->year;

        osMutexWait(mutexRTCHandle, osWaitForever);
        if (stmDate.Year < 36 &&
            stmDate.Year > 10) {  // sometimes timestamp is wrong and has
                                  // value like 2066 year
            HAL_RTC_SetTime(&hrtc, &stmTime, RTC_FORMAT_BIN);
            HAL_RTC_SetDate(&hrtc, &stmDate, RTC_FORMAT_BIN);
            HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);  // lock it in with the backup registers
            ret = 1;
        }
        osMutexRelease(mutexRTCHandle);
    }
    return ret;
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
