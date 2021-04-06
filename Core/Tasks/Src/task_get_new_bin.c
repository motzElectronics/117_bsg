#include "../Tasks/Inc/task_get_new_bin.h"
#include "../Tasks/Inc/task_keep_alive.h"
#include "../Utils/Inc/utils_pckgs_manager.h"

extern osThreadId webExchangeHandle;
extern osThreadId getGPSHandle;
extern osThreadId getNewBinHandle;
extern osThreadId keepAliveHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWebHandle;

extern CircularBuffer circBufAllPckgs;

u8 isRxNewFirmware = 0;
static u8 bufNumBytesFirmware[8];
static PckgUpdFirmware pckgInfoFirmware;
static u8 partFirmware[SZ_PART_FIRMW + 1];
static u32 flashAddrFirmware = FLASH_ADDR_BUF_NEW_FIRMWARE;
static u32 szSoft = 0;

void taskGetNewBin(void const* argument) {
    u32 curSzSoft = 0;
    u32 szPartSoft;
    u8 cntFailTCPReq = 0;

    FLASH_Erase_Sector(FLASH_SECTOR_1, VOLTAGE_RANGE_3);

    vTaskSuspend(getNewBinHandle);
    D(printf("taskCreateWebPckg\r\n"));

    lockAllTasks();
    isRxNewFirmware = 1;

    if (!bsg.isTCPOpen) {
        while (openTcp() != TCP_OK);
    }

    while (!(szSoft = getSzFirmware()));
    flashClearPage(FLASH_SECTOR_6);
    flashClearPage(FLASH_SECTOR_7);
    clearAllWebPckgs();

    for (;;) {
        if (szSoft != curSzSoft) {
            if (szSoft - curSzSoft > SZ_PART_FIRMW) {
                szPartSoft = SZ_PART_FIRMW;
            } else {
                szPartSoft = szSoft - curSzSoft;
            }
            pckgInfoFirmware.fromByte = curSzSoft;
            pckgInfoFirmware.toByte = szPartSoft + curSzSoft;
            memcpy(bufNumBytesFirmware, &pckgInfoFirmware.fromByte, 4);
            memcpy(bufNumBytesFirmware + 4, &pckgInfoFirmware.toByte, 4);
            memset(partFirmware, 0xFF, SZ_PART_FIRMW + 1);

            if (!bsg.isTCPOpen) {
                while (openTcp() != TCP_OK);
                cntFailTCPReq = 0;
            }

            osDelay(100);
            if (getPartFirmware(bufNumBytesFirmware, partFirmware, szPartSoft + 4, 8) == SUCCESS &&
                isCrcOk(partFirmware, szPartSoft)) {
                curSzSoft += szPartSoft;
                D(printf("OK: DOWNLOAD %d BYTES\r\n", (int)curSzSoft));
                // HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
                cntFailTCPReq = 0;
                flashWrite(partFirmware, szPartSoft, &flashAddrFirmware);
            } else {
                D(printf("ERROR: httpPost() DOWNLOAD\r\n"));
                cntFailTCPReq++;
                if (cntFailTCPReq > 10) {
                    cntFailTCPReq = 0;
                    simReset();
                }
            }
        } else {
            if (!bsg.isTCPOpen) {
                while (openTcp() != TCP_OK);
            }
            if (sendMsgFWUpdated() != SUCCESS) {
                D(printf("ERROR: Send FW UPDATED\r\n"));
            }
            D(printf("DOWNLOAD COMPLETE\r\n"));
            updBootInfo();
            osDelay(100);
            NVIC_SystemReset();
        }
    }
    /* USER CODE END taskGetNewBin */
}

void updBootInfo() {
    szSoft = szSoft % 4 == 0 ? szSoft : ((szSoft / 4) + 1) * 4;
    while (HAL_FLASH_Unlock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Unlock()\r\n"));
    FLASH_Erase_Sector(FLASH_SECTOR_1, VOLTAGE_RANGE_3);
    // D(printf("FLASH_Erase_Sector\r\n"));
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_ID_BOOT, BSG_ID_BOOT))
        D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_ID_LOADER)\r\n"));
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_IS_NEW_FIRWARE, (u32)1))
        D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_IS_NEW_FIRWARE)\r\n"));
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_SZ_NEW_FIRMWARE, (u32)(szSoft)))
        D(printf("ERROR: HAL_FLASH_Program(FLASH_ADDR_SZ_NEW_FIRMWARE)\r\n"));
    while (HAL_FLASH_Lock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Lock()\r\n"));
    D(printf("BOOT_ID: %d\r\n", (int)getFlashData(FLASH_ADDR_ID_BOOT)));
    D(printf("IS_NEW_FIRMARE: %d\r\n", (int)getFlashData(FLASH_ADDR_IS_NEW_FIRWARE)));
}

void lockAllTasks() {
    osMutexWait(mutexWebHandle, osWaitForever);

    vTaskSuspend(webExchangeHandle);
    vTaskSuspend(getGPSHandle);
    vTaskSuspend(keepAliveHandle);
    vTaskSuspend(createWebPckgHandle);

    osMutexRelease(mutexWebHandle);
}

u32 getSzFirmware() {
    u8 bufSzFirmware[4];
    if (generateWebPckgReq(CMD_REQUEST_SZ_FIRMWARE, NULL, 0, SZ_REQUEST_GET_SZ_FIRMWARE, bufSzFirmware, 4) == ERROR) {
        D(printf("ERROR: sz firmware\r\n"));
        return 0;
    } else {
        u32 numFirmware = bufSzFirmware[0] << 24 | bufSzFirmware[1] << 16 | bufSzFirmware[2] << 8 | bufSzFirmware[3];
        D(printf("OK: sz firmware %d\r\n", numFirmware));
        return numFirmware;
    }
}

ErrorStatus getPartFirmware(u8* reqData, u8* answBuf, u16 szAnsw, u8 szReq) {
    WebPckg* curPckg;
    ErrorStatus ret = SUCCESS;
    curPckg = createWebPckgReq(CMD_REQUEST_PART_FIRMWARE, reqData, szReq, SZ_REQUEST_GET_PART_FIRMWARE);
    osMutexWait(mutexWebHandle, osWaitForever);
    if (sendTcp(curPckg->buf, curPckg->shift) != TCP_OK) {
        D(printf("ERROR: part Firmware\r\n"));
        ret = ERROR;
    } else {
        waitIdleCnt("wait IDLE part firmware", &(uInfoSim.irqFlags), szAnsw / SZ_TCP_PCKG, 100, 10000);
        osDelay(100);
        memcpy(answBuf, &uInfoSim.pRxBuf[11], szAnsw);
    }
    osMutexRelease(mutexWebHandle);
    clearWebPckg(curPckg);
    return ret;
}
