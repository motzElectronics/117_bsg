#ifndef __TASK_TRAIN_DATA_H
#define __TASK_TRAIN_DATA_H

#include "cmsis_os.h"
#include "main.h"
#include "usart.h"

#define CMD_HDR_SIZE sizeof(cmd_header_t)
#define CMD_CRC_SIZE sizeof(uint16_t)

typedef enum {
    UPD_NONE = 0,
    UPD_START = 1,
    UPD_DOWNLOAD,
    UPD_FINISH
} UpdateStep;

typedef struct {
    u32        idMCU[3];
    u8         idFirmware;
    u8         idBoot;
    UpdateStep updStep;
    u32        fwCurSize;
    u32        fwSize;
} TabloInfo;

typedef enum {
    CMD_DATA = 1,
    CMD_TELEMETRY,
    CMD_CFG,
    CMD_SYNC,
    CMD_ERROR,
    CMD_NEW_FW = 16,
    CMD_GET_UID,
    CMD_GET_FW_NUM,
    CMD_FW_LEN,
    CMD_FW_PART,
    CMD_FW_END
} cmd_type_t;

typedef enum {
    ERROR_CAPACITY = 1,
    ERROR_NO_DATA
} cmd_err_type_t;

typedef __packed struct {
    u8  cmd_type;
    u16 length;
} cmd_header_t;

u8 tablo_send_request(u8 cmd, u8* data, u8 len);
u8 tablo_send_fw_part(u32 write_from, u8* data, u16 len);

#endif