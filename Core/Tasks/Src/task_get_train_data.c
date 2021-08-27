#include "../Tasks/Inc/task_get_train_data.h"

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/utils_crc.h"
#include "cmsis_os.h"

extern osThreadId    getTrainDataHandle;
extern osSemaphoreId uart6RXSemHandle;

extern UartInfo uInfoTablo;

extern CircularBuffer circBufAllPckgs;

u8 tmpBufPage[SZ_PAGE];

void tablo_send_pack(UartInfo* pUInf, u16 len);
u16  tablo_create_request(u8* request, u8 cmd, u8* data, u8 len);
u8   tablo_parse_responce(UartInfo* pUInf);

void taskGetTrainData(void const* argument) {
    u16 iter = 0;
    osSemaphoreWait(uart6RXSemHandle, osWaitForever);

    vTaskSuspend(getTrainDataHandle);

    while (!tablo_send_request(CMD_GET_UID, NULL, 0)) {}
    while (!tablo_send_request(CMD_GET_FW_NUM, NULL, 0) || bsg.tablo.idFirmware == 0) {}

    for (;;) {
        if (iter % 20 == 0) {
            u32 timeStamp = getUnixTimeStamp();
            tablo_send_request(CMD_SYNC, (u8*)&timeStamp, sizeof(timeStamp));
        } else if (iter % 20 == 10) {
            tablo_send_request(CMD_DATA, (u8*)&uInfoTablo.szRxBuf, sizeof(uInfoTablo.szRxBuf));
        } else if (iter % 200 == 5) {
            tablo_send_request(CMD_GET_FW_NUM, NULL, 0);
        }

        osDelay(1000);
        iter++;
    }
}

u8 tablo_send_request(u8 cmd, u8* data, u8 len) {
    u16 req_len;
    req_len = tablo_create_request(uInfoTablo.pTxBuf, cmd, data, len);

    tablo_send_pack(&uInfoTablo, req_len);
    if (osSemaphoreWait(uart6RXSemHandle, 5000) == osOK) {
        return tablo_parse_responce(&uInfoTablo);
    }
    return 0;
}

u16 tablo_create_request(u8* request, u8 cmd, u8* data, u8 len) {
    u16          ptr = 0;
    cmd_header_t hdr;

    LOG(LEVEL_MAIN, "tablo_create_request %d\r\n", cmd);

    hdr.cmd_type = cmd;
    hdr.length = len;
    memcpy(&request[ptr], &hdr, CMD_HDR_SIZE);
    ptr += CMD_HDR_SIZE;

    memcpy(&request[ptr], data, len);
    ptr += len;

    return ptr;
}

void tablo_send_pack(UartInfo* pUInf, u16 len) {
    uint16_t crc16;

    crc16 = calcCrc16(pUInf->pTxBuf, len);
    pUInf->pTxBuf[len] = crc16 & 0x00ff;
    pUInf->pTxBuf[len + 1] = (crc16 & 0xff00) >> 8;

    uart6Tx(pUInf->pTxBuf, len + CMD_CRC_SIZE, pUInf);
}

u8 tablo_send_fw_part(u32 write_from, u8* data, u16 len) {
    u16          ptr = 0;
    cmd_header_t hdr;
    u32          write_to;

    LOG(LEVEL_MAIN, "tablo send FW part %d %d\r\n", write_from, len);

    hdr.cmd_type = CMD_FW_PART;
    hdr.length = len + 4 + 4;
    memcpy(&uInfoTablo.pTxBuf[ptr], &hdr, CMD_HDR_SIZE);
    ptr += CMD_HDR_SIZE;

    memcpy(&uInfoTablo.pTxBuf[ptr], &write_from, sizeof(u32));
    ptr += sizeof(u32);
    write_to = write_from + len;
    memcpy(&uInfoTablo.pTxBuf[ptr], &write_to, sizeof(u32));
    ptr += sizeof(u32);
    memcpy(&uInfoTablo.pTxBuf[ptr], data, len);
    ptr += len;

    tablo_send_pack(&uInfoTablo, ptr);
    if (osSemaphoreWait(uart6RXSemHandle, 5000) == osOK) {
        return tablo_parse_responce(&uInfoTablo);
    }
    return 0;
}

void parse_tablo_data(u8* data_all, u16 len_all) {
    u16 ptr = 0;
    u16 len_pg;
    u8* data;
    u16 bufEnd[2] = {0, BSG_PREAMBLE};

    while (ptr < len_all) {
        len_pg = (data_all[ptr] + (data_all[ptr + 1] << 8));
        ptr += 2;
        data = (u8*)(&data_all[ptr]);

        if (len_pg > SZ_PAGE) {
            LOG(LEVEL_ERROR, "Returned big page (len %d)\r\n", len_pg);
            return;
        }

        memset(tmpBufPage, 0, SZ_PAGE);
        memcpy(tmpBufPage, data, len_pg);
        bufEnd[0] = calcCrc16(data, len_pg);
        memcpy(&tmpBufPage[len_pg], bufEnd, 4);
        spiFlashWrPg(tmpBufPage, len_pg + 4, 0, spiFlash64.headNumPg);

        LOG(LEVEL_INFO, "Returned page (len %d)\r\n", len_pg);

        ptr += len_pg;
    }
}

void parse_tablo_sync(u8* data_all, u16 len_all) {
    LOG(LEVEL_INFO, "Time syncronized\r\n");
}

void parse_tablo_error(u8* data_all, u16 len_all) {
    u8 error_type = data_all[0];

    LOG(LEVEL_INFO, "Returned error type %d\r\n", error_type);
}

void parse_tablo_new_fw(u8* data_all, u16 len_all) {
    LOG(LEVEL_INFO, "FW download sterted\r\n");
}

void parse_tablo_uid(u8* data_all, u16 len_all) {
    memcpy(&bsg.tablo.idMCU, data_all, 12);
    LOG(LEVEL_INFO, "Returned Tablo UID ");
    printf("%08x%08x%08x\r\n", (uint)bsg.tablo.idMCU[0], (uint)bsg.tablo.idMCU[1], (uint)bsg.tablo.idMCU[2]);
}

void parse_tablo_fw_num(u8* data_all, u16 len_all) {
    bsg.tablo.idFirmware = data_all[0];
    LOG(LEVEL_INFO, "Returned Tablo FW Num %d\r\n", bsg.tablo.idFirmware);
}

void parse_tablo_fw_len(u8* data_all, u16 len_all) {
    bsg.tablo.fwSize = *(u32*)data_all;
    bsg.tablo.fwCurSize = 0;
    bsg.tablo.updStep = UPD_DOWNLOAD;
    LOG(LEVEL_INFO, "Returned Tablo FW Len %d\r\n", bsg.tablo.fwSize);
}

void parse_tablo_fw_part(u8* data_all, u16 len_all) {
    u32 write_from, write_to;

    write_from = *(u32*)data_all;
    write_to = *(u32*)(data_all + 4);
    bsg.tablo.fwCurSize = *(u32*)(data_all + 8);
    bsg.tablo.updStep = UPD_DOWNLOAD;

    LOG(LEVEL_INFO, "Returned Tablo FW Part %d %d\r\n", write_from, write_to, bsg.tablo.fwCurSize);
}

void parse_tablo_fw_end(u8* data_all, u16 len_all) {
    bsg.tablo.updStep = UPD_FINISH;
    LOG(LEVEL_INFO, "Returned Tablo FW Len %d\r\n", bsg.tablo.fwSize);
}

u8 tablo_parse_responce(UartInfo* pUInf) {
    cmd_header_t* hdr = (cmd_header_t*)pUInf->pRxBuf;
    u8*           data;
    u16           crc_calc;

    crc_calc = calcCrc16(pUInf->pRxBuf, (hdr->length + CMD_HDR_SIZE + CMD_CRC_SIZE));
    if (crc_calc != 0) {
        LOG(LEVEL_ERROR, "Bad crc\r\n");
        return 0;
    }

    data = pUInf->pRxBuf + CMD_HDR_SIZE;
    data[hdr->length] = 0;

    switch (hdr->cmd_type) {
        case CMD_DATA:
            parse_tablo_data(data, hdr->length);
            break;
        case CMD_TELEMETRY:
            break;
        case CMD_CFG:
            break;
        case CMD_SYNC:
            parse_tablo_sync(data, hdr->length);
            break;
        case CMD_ERROR:
            parse_tablo_error(data, hdr->length);
            return 0;
            break;
        case CMD_NEW_FW:
            parse_tablo_new_fw(data, hdr->length);
            break;
        case CMD_GET_UID:
            parse_tablo_uid(data, hdr->length);
            break;
        case CMD_GET_FW_NUM:
            parse_tablo_fw_num(data, hdr->length);
            break;
        case CMD_FW_LEN:
            parse_tablo_fw_len(data, hdr->length);
            break;
        case CMD_FW_PART:
            parse_tablo_fw_part(data, hdr->length);
            break;
        case CMD_FW_END:
            parse_tablo_fw_end(data, hdr->length);
            break;
        default:
            return 0;
            break;
    }
    return 1;
}