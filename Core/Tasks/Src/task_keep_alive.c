#include "../Tasks/Inc/task_keep_alive.h"

#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/utils_pckgs_manager.h"

extern osThreadId    webExchangeHandle;
extern osThreadId    getGPSHandle;
extern osThreadId    keepAliveHandle;
extern osThreadId    getNewBinHandle;
extern osThreadId    createWebPckgHandle;
extern osMutexId     mutexWebHandle;
extern osMessageQId  queueWebPckgHandle;
extern osSemaphoreId semCreateWebPckgHandle;

extern u8 isRxNewFirmware;

extern CircularBuffer circBufAllPckgs;

u8 bufTxData[20];

void taskKeepAlive(void const* argument) {
    u16 timeout = 1;
    vTaskSuspend(keepAliveHandle);

    for (;;) {
        if ((timeout % 120 == 10) && !isRxNewFirmware) {
            D(printf("getBsgNumFw\r\n\r\n"));
            getBsgNumFw();
        }
        if ((timeout % 120 == 20) && !isRxNewFirmware) {
            D(printf("getTabloNumFw\r\n\r\n"));
            getTabloNumFw();
        }
        // if (!(timeout % 30) && !isRxNewFirmware) {
        //     D(printf("sendMsgGpsInvalidCount\r\n"));
        //     sendMsgGpsInvalidCount();
        // }
        if (!(timeout % 600) && !isRxNewFirmware) {
            D(printf("\r\ngenerateMsgKeepAlive\r\n\r\n"));
            generateMsgKeepAlive();
        }
        if (!(timeout % 5400) && !isRxNewFirmware) {
            D(printf("\r\nupdRTC\r\n\r\n"));
            updRTC();
        }

        timeout++;
        osDelay(2000);
    }
}

void updRTC() {
    getServerTime();
}

void generateMsgKeepAlive() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BSG_ALIVE;
    pckgTel.data = 0;
    saveTelemetry(&pckgTel, &circBufAllPckgs);
}

void generateMsgFWUpdated() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_UPDATED;
    pckgTel.data = 0;
    saveTelemetry(&pckgTel, &circBufAllPckgs);
}

void generateMsgDevOff() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BSG;
    pckgTel.data = 0;
    saveTelemetry(&pckgTel, &circBufAllPckgs);
}

ErrorStatus sendMsgFWUpdated() {
    ErrorStatus   ret = SUCCESS;
    PckgTelemetry pckgTel;
    u8*           idMCU;

    if (bsg.updTarget == UPD_TARGET_TABLO) {
        idMCU = (u8*)&bsg.tablo.info.idMCU;
    } else if (bsg.updTarget == UPD_TARGET_BSG) {
        idMCU = (u8*)&bsg.idMCU;
    } else {
        return ERROR;
    }

    D(printf("sendMsgFWUpdated\r\n"));

    memset(bufTxData, 0, 20);
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_UPDATED;
    pckgTel.data = 1;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(bufTxData, &pckgTel);

    pckgTel.code = TEL_CD_HW_BSG;
    pckgTel.data = 0;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY], &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY * 2, 2, idMCU);

    return ret;
}

ErrorStatus sendMsgFWUpdateBegin() {
    ErrorStatus   ret = SUCCESS;
    PckgTelemetry pckgTel;
    u8            ptr = 0;
    u8*           idMCU;

    if (bsg.updTarget == UPD_TARGET_TABLO) {
        idMCU = (u8*)&bsg.tablo.info.idMCU;
    } else if (bsg.updTarget == UPD_TARGET_BSG) {
        idMCU = (u8*)&bsg.idMCU;
    } else {
        return ERROR;
    }

    D(printf("sendMsgFWUpdated\r\n"));

    memset(bufTxData, 0, 64);
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_UPDATED;
    pckgTel.data = bsg.idNewFirmware;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);
    pckgTel.code = TEL_CD_HW_UPDATE_LEN;
    pckgTel.data = bsg.szNewFirmware;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY * ptr, ptr, idMCU);

    return ret;
}

ErrorStatus sendMsgDevOff() {
    ErrorStatus   ret = SUCCESS;
    PckgTelemetry pckgTel;

    memset(bufTxData, 0, 20);
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BSG;
    pckgTel.data = 0;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(bufTxData, &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY, 1, &bsg.idMCU);

    return ret;
}

ErrorStatus sendMsgGpsInvalidCount() {
    ErrorStatus   ret = SUCCESS;
    PckgTelemetry pckgTel;

    memset(bufTxData, 0, 20);
    pckgTel.group = TEL_GR_GENINF;
    pckgTel.code = TEL_CD_GENINF_GPS_INV_CNT;
    pckgTel.data = bsg.gpsInvaligCount;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(bufTxData, &pckgTel);

    pckgTel.code = TEL_CD_GENINF_GPS_PARSE_ER_CNT;
    pckgTel.data = bsg.gpsParseFailCount;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY], &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY * 2, 2, &bsg.idMCU);

    return ret;
}

// void powerOffBsg()
// {
//     char strVolts[4];
//     static u32 delayPages = 1;
//     u8 cnt;

//     vTaskSuspend(getGPSHandle);
//     vTaskSuspend(keepAliveHandle);

//     osDelay(2000);
//     D(printf("OK: PWR OFF START\r\n"));
//     D(printf("OK: PWR OFF WAIT: %d\r\n", getUnixTimeStamp()));

//     while (delayPages > BSG_THRESHOLD_CNT_PAGES) {
//         osDelay(5000);
//         printf("delayPages %d\r\n", delayPages);
//         delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ?
//                     spiFlash64.headNumPg - spiFlash64.tailNumPg :
//                     spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
//     }

//     while ((cnt = getCntFreePckg()) != CNT_WEBPCKGS) {
//         osDelay(5000);
//         printf("getCntFreePckg %d\r\n", cnt);
//     }

//     generateMsgFWUpdated();
//     generateMsgDevOff();
//     D(printf("OFF  VOLT: %d\r\n", bkte.pwrInfo.adcVoltBat));
//     bkte.isSentData = 0;
//     updSpiFlash(&circBufAllPckgs);
//     xSemaphoreGive(semCreateWebPckgHandle);

//     while (!bkte.isSentData) {
//         osDelay(1000);
//         printf("wait isSentData\r\n");
//     }

//     D(printf("OK: PWR OFF SENT TELEMETRY: %d\r\n", getUnixTimeStamp()));

//     osDelay(5000);
//     NVIC_SystemReset();
// }