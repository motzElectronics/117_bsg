#ifndef __TASK_TRAIN_DATA_H
#define __TASK_TRAIN_DATA_H

#include "cmsis_os.h"
#include "main.h"
#include "usart.h"

#define CMD_HDR_SIZE sizeof(cmd_header_t)
#define CMD_CRC_SIZE sizeof(uint16_t)

typedef enum {
    CMD_DATA = 1,
    CMD_TELEMETRY,
    CMD_CFG,
    CMD_SYNC,
    CMD_ERROR
} cmd_type_t;

typedef enum {
    ERROR_CAPACITY = 1,
    ERROR_NO_DATA
} cmd_err_type_t;

typedef __packed struct {
    u8  cmd_type;
    u16 length;
} cmd_header_t;

#endif