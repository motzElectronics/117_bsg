/*
 * m_spiflash.c
 *
 *  Created on: 11 июн. 2020 г.
 *      Author: YOBA-i516G
 */

#include "../xDrvrs/Inc/spiflash.h"
SPIFlash spiFlash64;

extern osMutexId mutexSpiFlashHandle;

void spiFlashInit(u8* buf) {
    osMutexWait(mutexSpiFlashHandle, 60000);
    spiMemInfo.pHSpi = &hspi2;
    SPIFLASH_CS_UNSEL;
    osDelay(100);
    u32 id, tmp;

    id = spiFlashReadID();
    tmp = id & 0x0000FFFF;
    if (tmp) {  //! change to 2017
                // bsg.hwStat.isSPIFlash = 1;
    }
    D(printf("spiFlashId, series: %d %x\r\n", (int)id, (int)tmp));

    spiFlash64.blCnt = 128;
    spiFlash64.pgSz = 256;
    spiFlash64.secSz = 0x1000;
    spiFlash64.secCnt = spiFlash64.blCnt * 16;
    spiFlash64.pgCnt = (spiFlash64.secCnt * spiFlash64.secSz) / spiFlash64.pgSz;
    spiFlash64.blSz = spiFlash64.secSz * 16;
    spiFlash64.capacityKb = (spiFlash64.secCnt * spiFlash64.secSz) / 1024;
    spiFlash64.headNumPg = 0;
    osMutexRelease(mutexSpiFlashHandle);
    spiFlashRdPg(buf, 256, 0, BSG_SAVE_NUM_PAGE);
    tmp = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
    if (tmp < SPIFLASH_NUM_PG_GNSS)
        spiFlash64.headNumPg = tmp;
    spiFlash64.tailNumPg = spiFlash64.headNumPg;
    spiFlashES(spiFlash64.headNumPg / SPIFLASH_NUM_PG_IN_SEC);
    //TODO:бинарный поиск head, tail

    //	for(u8 i = 0; i < SPIFLASH_NUM_SEC_GNSS; i++)
    //		spiFlashES(i);
}

u32 spiFlashReadID(void) {
    u32 id;
    u8  tmp[4] = {SPIFLASH_CMD_JEDEC_ID, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE};
    SPIFLASH_CS_SEL;
    spiFlashTxRxCmd(tmp, sizeof(tmp));
    SPIFLASH_CS_UNSEL;
    id = (tmp[1] << 16) | (tmp[2] << 8) | tmp[3];
    //	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
    return id;
}

u8 spiFlashTxRxCmd(u8* data, u16 sz) {
    static u8 ret = 0;
    spiMemInfo.irqFlags.regIrq = 0;
    HAL_SPI_TransmitReceive_DMA(spiMemInfo.pHSpi, data, data, sz);  // spi2
    ret = waitRx("", &spiMemInfo.irqFlags, 50, WAIT_TIMEOUT);
    // bsg.erFlags.flashNOIRQ = ~ret;
    return ret;
}

u8 spiFlashTxData(u8* data, u16 sz) {
    static u8 ret = 0;
    spiMemInfo.irqFlags.regIrq = 0;
    HAL_SPI_Transmit_DMA(spiMemInfo.pHSpi, data, sz);  // spi2
    ret = waitTx("", &spiMemInfo.irqFlags, 50, WAIT_TIMEOUT);
    // bsg.erFlags.flashNOIRQ = ~ret;
    return ret;
}

u8 spiFlashRxData(u8* data, u16 sz) {
    static u8 ret = 0;
    spiMemInfo.irqFlags.regIrq = 0;
    HAL_SPI_Receive_DMA(spiMemInfo.pHSpi, data, sz);  // spi2
    ret = waitRx("", &spiMemInfo.irqFlags, 50, WAIT_TIMEOUT);
    // bsg.erFlags.flashNOIRQ = ~ret;
    return ret;
}

void spiFlashES(u32 numSec) {
    u32 secAddr;
    // D(printf("spiFlash ErSec %d\r\n", numSec));

    osMutexWait(mutexSpiFlashHandle, 60000);

    secAddr = numSec * spiFlash64.secSz;
    u8 data[] = {SPIFLASH_SE, ((secAddr & 0xFF0000) >> 16), ((secAddr & 0xFF00) >> 8), (secAddr & 0xFF)};

    spiFlashWaitReady();
    spiFlashWrEn();

    SPIFLASH_CS_SEL;
    spiFlashTxRxCmd(data, 4);
    SPIFLASH_CS_UNSEL;

    spiFlashWaitReady();
    osDelay(10);
    osMutexRelease(mutexSpiFlashHandle);
}

u8 spiFlashWaitReady() {
    u8 data[] = {SPIFLASH_RDSR, DUMMY_BYTE};
    osDelay(50);
    u8 i = 0, ret = 0;
    do {
        SPIFLASH_CS_SEL;
        osDelay(50);
        ret = spiFlashTxRxCmd(data, sizeof(data));
        i++;
        SPIFLASH_CS_UNSEL;
        osDelay(50);
    } while (((data[1] & 0x01) == 0x01) && (i < 3));
    return ret;
}

void spiFlashWrEn() {
    u8 cmd = SPIFLASH_WREN;
    SPIFLASH_CS_SEL;
    spiFlashTxRxCmd(&cmd, 1);
    SPIFLASH_CS_UNSEL;
    osDelay(10);
}

u8 spiFlashWrPg(u8* pBuf, u32 sz, u32 offset, u32 numPage) {
    D(printf("spiFlash WrPg %d\r\n", numPage));
    u32 addr;
    u8  ret = 0;
    if (spiFlash64.tailNumPg > spiFlash64.headNumPg &&
        (spiFlash64.tailNumPg - spiFlash64.headNumPg) < SPIFLASH_NUM_PG_IN_SEC && !(spiFlash64.headNumPg % SPIFLASH_NUM_PG_IN_SEC)) {
        spiFlash64.tailNumPg = (spiFlash64.tailNumPg + (SPIFLASH_NUM_PG_IN_SEC - (spiFlash64.tailNumPg % SPIFLASH_NUM_PG_IN_SEC)) + 1) % SPIFLASH_NUM_PG_GNSS;
        ret = 1;
    }

    if (!spiFlash64.headNumPg)
        spiFlashES(0);
    else if (spiFlash64.headNumPg % SPIFLASH_NUM_PG_IN_SEC == 0)
        spiFlashES(spiFlash64.headNumPg / SPIFLASH_NUM_PG_IN_SEC);

    osMutexWait(mutexSpiFlashHandle, 60000);

    if ((offset + sz) > spiFlash64.pgSz)
        sz = spiFlash64.pgSz - offset;

    addr = (numPage * spiFlash64.pgSz) + offset;
    u8 data[] = {SPIFLASH_PP, ((addr & 0xFF0000) >> 16), ((addr & 0xFF00) >> 8), (addr & 0xFF)};
    // D(printf("spiFlashWaitReady()\r\n"));
    spiFlashWaitReady();
    // D(printf("spiFlashWrEn()\r\n"));
    spiFlashWrEn();

    SPIFLASH_CS_SEL;
    // D(printf("SPIFLASH_PP()\r\n"));
    spiFlashTxRxCmd(data, 4);
    // D(printf("spiFlashTxData()\r\n"));
    spiFlashTxData(pBuf, sz);
    SPIFLASH_CS_UNSEL;

    // D(printf("spiFlashWaitReady()\r\n"));
    spiFlashWaitReady();
    osDelay(10);
    spiFlash64.headNumPg = (spiFlash64.headNumPg + 1) % SPIFLASH_NUM_PG_GNSS;

    osMutexRelease(mutexSpiFlashHandle);
    return ret;
}

void spiFlashRdPg(u8* pBuf, u32 sz, u32 offset, u32 numPage) {
    D(printf("spiFlash RdPg %d\r\n", numPage));
    u32 addr;

    osMutexWait(mutexSpiFlashHandle, 60000);

    if ((offset + sz) > spiFlash64.pgSz)
        sz = spiFlash64.pgSz - offset;

    addr = (numPage * spiFlash64.pgSz) + offset;
    u8 data[] = {SPIFLASH_FAST_READ, ((addr & 0xFF0000) >> 16), ((addr & 0xFF00) >> 8), (addr & 0xFF), DUMMY_BYTE};

    SPIFLASH_CS_SEL;
    spiFlashTxRxCmd(data, 5);
    spiFlashRxData(pBuf, sz);
    SPIFLASH_CS_UNSEL;

    osDelay(10);
    spiFlash64.tailNumPg = (spiFlash64.tailNumPg + 1) % SPIFLASH_NUM_PG_GNSS;
    osMutexRelease(mutexSpiFlashHandle);
}
