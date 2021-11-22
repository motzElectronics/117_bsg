#include "../Tasks/Inc/task_get_new_bin.h"

#include "../Tasks/Inc/task_get_train_data.h"
#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_keep_alive.h"
#include "../Utils/Inc/utils_crc.h"
#include "../Utils/Inc/utils_pckgs_manager.h"

extern u16 iwdgTaskReg;

extern osThreadId webExchangeHandle;
extern osThreadId getGPSHandle;
extern osThreadId getNewBinHandle;
extern osThreadId keepAliveHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId getTrainDataHandle;

extern osMutexId mutexGPSBufHandle;
extern osMutexId mutexWebHandle;
extern osMutexId mutexSessionHandle;
extern osMutexId mutexRTCHandle;
extern osMutexId mutexSpiFlashHandle;

extern CircularBuffer circBufAllPckgs;

u8                     isRxNewFirmware = 0;
static u8              bufNumBytesFirmware[8];
static PckgUpdFirmware pckgInfoFirmware;
static u8              partFirmware[SZ_PART_FIRMW + 1];
static u32             flashAddrFirmware = FLASH_ADDR_BUF_NEW_FIRMWARE;
static u32             szSoft = 0;
static u32             crcNewFW;

void big_update_func();

void taskGetNewBin(void const* argument) {
    u8 updateFailCnt = 0;
    FLASH_Erase_Sector(FLASH_SECTOR_2, VOLTAGE_RANGE_3);

    vTaskSuspend(getNewBinHandle);
    LOG(LEVEL_MAIN, "taskGetNewBin\r\n");

    lockAllTasks();
    isRxNewFirmware = 1;

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_NEW_BIN;
        big_update_func();
        osDelay(5000);
        updateFailCnt++;
        if (updateFailCnt == 5) {
            NVIC_SystemReset();
        }
    }

    /* USER CODE END taskGetNewBin */
}

void big_update_func() {
    u32 curSzSoft = 0;
    u32 szPartSoft;
    u8  cntFailTCPReq = 0;
    u8  cntFailTablo = 0;
    u8  res;

    osMutexWait(mutexWebHandle, osWaitForever);
    closeTcp();
    osMutexRelease(mutexWebHandle);
    while (!(szSoft = getSzFirmware())) {}

    flashClearPage(FLASH_SECTOR_6);
    flashClearPage(FLASH_SECTOR_7);
    clearAllWebPckgs();

    crcNewFW = 0xffffffff;

    if (bsg.updTarget == UPD_TARGET_TABLO) {
        res = 0;
        cntFailTablo = 0;
        while (!res) {
            res = tablo_send_request(CMD_NEW_FW, NULL, 0);
            cntFailTablo++;
            if (cntFailTablo > 5) {
                return;
            }
        }

        res = 0;
        cntFailTablo = 0;
        while (!res || szSoft != bsg.tablo.fwSize) {
            res = tablo_send_request(CMD_FW_LEN, (u8*)&szSoft, sizeof(u32));
            cntFailTablo++;
            if (cntFailTablo > 5) {
                return;
            }
        }
    }

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_NEW_BIN;
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

            osMutexWait(mutexWebHandle, osWaitForever);
            if (!bsg.isTCPOpen) {
                while (openTcp(SERVER_MOTZ) != TCP_OK) {}
                cntFailTCPReq = 0;
            }
            osMutexRelease(mutexWebHandle);

            osDelay(100);
            if (getPartFirmware(bufNumBytesFirmware, partFirmware, szPartSoft + 4, 8) == SUCCESS &&
                isCrcOk(partFirmware, szPartSoft)) {
                crc32_chank(&crcNewFW, partFirmware, szPartSoft);
                cntFailTCPReq = 0;

                if (bsg.updTarget == UPD_TARGET_TABLO) {
                    res = 0;
                    cntFailTablo = 0;
                    while (!res || (curSzSoft + szPartSoft) != bsg.tablo.fwCurSize) {
                        res = tablo_send_fw_part(curSzSoft, partFirmware, szPartSoft);
                        cntFailTablo++;
                        if (cntFailTablo > 5) {
                            return;
                        }
                    }
                } else if (bsg.updTarget == UPD_TARGET_BSG) {
                    flashWrite(partFirmware, szPartSoft, &flashAddrFirmware);
                }

                curSzSoft += szPartSoft;
                LOG(LEVEL_MAIN, "DOWNLOAD %d BYTES addr 0x%08x\r\n", (int)curSzSoft, flashAddrFirmware);
            } else {
                LOG(LEVEL_ERROR, "httpPost() DOWNLOAD\r\n");
                cntFailTCPReq++;
                if (cntFailTCPReq > 10) {
                    cntFailTCPReq = 0;
                    // simReset();
                    return;
                }
            }
        } else {
            osMutexWait(mutexWebHandle, osWaitForever);
            if (!bsg.isTCPOpen) {
                while (openTcp(SERVER_MOTZ) != TCP_OK) {}
            }
            osMutexRelease(mutexWebHandle);
            if (bsg.updTarget == UPD_TARGET_TABLO) {
                if (sendMsgTabloFW() != SUCCESS) {
                    LOG(LEVEL_ERROR, "Send TABLO FW UPDATED\r\n");
                }
            } else if (bsg.updTarget == UPD_TARGET_BSG) {
                if (sendMsgFWUpdated() != SUCCESS) {
                    LOG(LEVEL_ERROR, "Send FW UPDATED\r\n");
                }
            }
            LOG(LEVEL_MAIN, "DOWNLOAD COMPLETE\r\n");
            osMutexWait(mutexWebHandle, osWaitForever);
            closeTcp();
            osMutexRelease(mutexWebHandle);
            u8 diff = 4 - (szSoft % 4);
            if (diff > 0 && diff < 4) {
                memset(partFirmware, 0xFF, diff);
                crc32_chank(&crcNewFW, partFirmware, diff);
                // szNewFW += diff;
            }
            crcNewFW ^= 0xffffffff;
            LOG(LEVEL_MAIN, "crcNewFW 0x%08x\r\n", crcNewFW);

            if (bsg.updTarget == UPD_TARGET_TABLO) {
                res = 0;
                cntFailTablo = 0;
                while (!res) {
                    res = tablo_send_request(CMD_FW_END, (u8*)&crcNewFW, sizeof(u32));
                    cntFailTablo++;
                    if (cntFailTablo > 5) {
                        return;
                    }
                }
            } else if (bsg.updTarget == UPD_TARGET_BSG) {
                updBootInfo();
            }

            osDelay(1000);
            NVIC_SystemReset();
        }
    }
}

void updBootInfo() {
    szSoft = szSoft % 4 == 0 ? szSoft : ((szSoft / 4) + 1) * 4;
    while (HAL_FLASH_Unlock() != HAL_OK) LOG_FLASH(LEVEL_ERROR, "HAL_FLASH_Unlock()\r\n");
    FLASH_Erase_Sector(FLASH_SECTOR_2, VOLTAGE_RANGE_3);
    LOG_FLASH(LEVEL_MAIN, "FLASH_Erase_Sector\r\n");
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_ID_BOOT, BSG_ID_BOOT))
        LOG_FLASH(LEVEL_ERROR, "HAL_FLASH_Program(BOOT_ADDR_ID_LOADER)\r\n");
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_IS_NEW_FIRMWARE, (u32)1))
        LOG_FLASH(LEVEL_ERROR, "HAL_FLASH_Program(BOOT_ADDR_IS_NEW_FIRWARE)\r\n");
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_SZ_NEW_FIRMWARE, (u32)(szSoft)))
        LOG_FLASH(LEVEL_ERROR, "HAL_FLASH_Program(FLASH_ADDR_SZ_NEW_FIRMWARE)\r\n");
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_CRC_NEW_FIRMWARE, (u32)(crcNewFW)))
        LOG_FLASH(LEVEL_ERROR, "HAL_FLASH_Program(FLASH_ADDR_CRC_NEW_FIRMWARE)\r\n");

    while (HAL_FLASH_Lock() != HAL_OK) LOG_FLASH(LEVEL_ERROR, "HAL_FLASH_Lock()\r\n");
    LOG_FLASH(LEVEL_MAIN, "BOOT_ID: %d\r\n", (int)getFlashData(FLASH_ADDR_ID_BOOT));
    LOG_FLASH(LEVEL_MAIN, "IS_NEW_FIRMARE: %d\r\n", (int)getFlashData(FLASH_ADDR_IS_NEW_FIRMWARE));
    LOG_FLASH(LEVEL_MAIN, "SZ_NEW_FIRMWARE: %d\r\n", (int)getFlashData(FLASH_ADDR_SZ_NEW_FIRMWARE));
    LOG_FLASH(LEVEL_MAIN, "CRC_NEW_FIRMARE: 0x%08x\r\n", (int)getFlashData(FLASH_ADDR_CRC_NEW_FIRMWARE));
}

void lockAllTasks() {
    osMutexWait(mutexGPSBufHandle, osWaitForever);
    osMutexWait(mutexRTCHandle, osWaitForever);
    osMutexWait(mutexSpiFlashHandle, osWaitForever);
    osMutexWait(mutexWebHandle, osWaitForever);
    osMutexWait(mutexSessionHandle, osWaitForever);

    vTaskSuspend(webExchangeHandle);
    vTaskSuspend(getGPSHandle);
    vTaskSuspend(keepAliveHandle);
    vTaskSuspend(createWebPckgHandle);
    vTaskSuspend(getTrainDataHandle);

    osMutexRelease(mutexGPSBufHandle);
    osMutexRelease(mutexRTCHandle);
    osMutexRelease(mutexSpiFlashHandle);
    osMutexRelease(mutexWebHandle);
    osMutexRelease(mutexSessionHandle);
}

u32 getSzFirmware() {
    u8  bufSzFirmware[4];
    u8* idMCU;
    if (bsg.updTarget == UPD_TARGET_TABLO) {
        idMCU = (u8*)&bsg.tablo.info.idMCU;
    } else if (bsg.updTarget == UPD_TARGET_BSG) {
        idMCU = (u8*)&bsg.idMCU;
    } else {
        return ERROR;
    }

    if (generateWebPckgReq(CMD_REQUEST_SZ_FIRMWARE, NULL, 0, SZ_REQUEST_GET_SZ_FIRMWARE, bufSzFirmware, 4, idMCU) == ERROR) {
        LOG(LEVEL_ERROR, "sz firmware\r\n");
        return 0;
    } else {
        u32 numFirmware = bufSzFirmware[0] << 24 | bufSzFirmware[1] << 16 | bufSzFirmware[2] << 8 | bufSzFirmware[3];
        LOG(LEVEL_INFO, "OK: sz firmware %d\r\n", numFirmware);
        return numFirmware;
    }
}

ErrorStatus getPartFirmware(u8* reqData, u8* answBuf, u16 szAnsw, u8 szReq) {
    WebPckg*    curPckg;
    ErrorStatus ret = SUCCESS;

    u8* idMCU;
    if (bsg.updTarget == UPD_TARGET_TABLO) {
        idMCU = (u8*)&bsg.tablo.info.idMCU;
    } else if (bsg.updTarget == UPD_TARGET_BSG) {
        idMCU = (u8*)&bsg.idMCU;
    } else {
        return ERROR;
    }

    curPckg = createWebPckgReq(CMD_REQUEST_PART_FIRMWARE, reqData, szReq, SZ_REQUEST_GET_PART_FIRMWARE, idMCU);
    osMutexWait(mutexWebHandle, osWaitForever);
    if (sendTcp(SERVER_MOTZ, curPckg->buf, curPckg->shift) != TCP_OK) {
        LOG(LEVEL_ERROR, "part Firmware\r\n");
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
