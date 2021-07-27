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

void send_request(UartInfo* pUInf, u16 len);
u16  create_request(u8* request, u8 cmd, u8* data, u8 len);
void parse_responce(UartInfo* pUInf);

void taskGetTrainData(void const* argument) {
    u16 req_len;
    u16 iter = 0;
    osSemaphoreWait(uart6RXSemHandle, osWaitForever);

    vTaskSuspend(getTrainDataHandle);

    for (;;) {
        if (iter % 20 == 0) {
            u32 timeStamp = getUnixTimeStamp();
            req_len = create_request(uInfoTablo.pTxBuf, CMD_SYNC, (u8*)&timeStamp, sizeof(timeStamp));
            send_request(&uInfoTablo, req_len);
            if (osSemaphoreWait(uart6RXSemHandle, 2000) == osOK) {
                parse_responce(&uInfoTablo);
                osDelay(100);
            }
        }

        if (iter % 20 == 10) {
            req_len = create_request(uInfoTablo.pTxBuf, CMD_DATA, (u8*)&uInfoTablo.szRxBuf, sizeof(uInfoTablo.szRxBuf));
            send_request(&uInfoTablo, req_len);
            if (osSemaphoreWait(uart6RXSemHandle, 2000) == osOK) {
                parse_responce(&uInfoTablo);
            }
        }

        osDelay(100);
        iter++;
    }
}

void send_request(UartInfo* pUInf, u16 len) {
    uint16_t crc16;

    crc16 = calcCrc16(pUInf->pTxBuf, len);
    pUInf->pTxBuf[len] = crc16 & 0x00ff;
    pUInf->pTxBuf[len + 1] = (crc16 & 0xff00) >> 8;

    uart6Tx(pUInf->pTxBuf, len + CMD_CRC_SIZE, pUInf);
}

u16 create_request(u8* request, u8 cmd, u8* data, u8 len) {
    uint8_t      ptr = 0;
    cmd_header_t hdr;

    LOG(LEVEL_MAIN, "create_request %d\r\n", cmd);

    hdr.cmd_type = cmd;
    hdr.length = len;
    memcpy(&request[ptr], &hdr, CMD_HDR_SIZE);
    ptr += CMD_HDR_SIZE;

    memcpy(&request[ptr], data, len);
    ptr += len;

    return ptr;
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

void parse_responce(UartInfo* pUInf) {
    cmd_header_t* hdr = (cmd_header_t*)pUInf->pRxBuf;
    u8*           data;
    u16           crc_calc;

    crc_calc = calcCrc16(pUInf->pRxBuf, (hdr->length + CMD_HDR_SIZE + CMD_CRC_SIZE));
    if (crc_calc != 0) {
        LOG(LEVEL_ERROR, "Bad crc\r\n");
        return;
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
            break;
        default:
            break;
    }
}