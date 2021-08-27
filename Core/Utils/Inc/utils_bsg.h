/*
 * utils_bsg.h
 *
 *  Created on: Apr 20, 2020
 *      Author: bls
 */

#ifndef INC_UTILS_BSG_H_
#define INC_UTILS_BSG_H_

#include "../Tasks/Inc/task_get_train_data.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_crc.h"
#include "../xDrvrs/Inc/simcom.h"
#include "../xDrvrs/Inc/spiflash.h"
#include "main.h"
#include "time.h"
#include "usart.h"

#define BSG_THRESHOLD_CNT_PAGES 2
#define BSG_ID_DEV_BSG          0x12
#define BSG_ADDR_ID_MCU         0x1FFF7A10

#define SZ_PART_NEW_SOFTWARE 1360
#define SZ_MAX_TX_DATA       4096

#define BSG_MSG_NO_GPS (char*)"0000.000000,N,00000.000000,E,+0000,000,000"

#define BSG_ID_FIRMWARE         8
#define BSG_ID_BOOT             2
#define BSG_ID_TRAINCAR         0
#define BSG_ID_TRAIN            1706
#define BSG_VER_BETA_FIRMWARE   (char)'B'
#define BSG_VER_STABLE_FIRMWARE (char)'S'

#define BSG_CNT_INVALID_GNSS_DATA 5

#define BSG_SZ_BUF_PCKG_GNSS 512

#define BSG_SAVE_NUM_PAGE 2047

#define LEN_TIMESTAMP 13

#define BSG_BIG_DIF_RTC_SERVTIME 600

#define UPD_TARGET_BSG   0
#define UPD_TARGET_TABLO 1

typedef union {
    struct {
        u8 cntr   : 3;
        u8 flagOn : 1;
    };
    u8 regSleepTimer;
} SleepTimer;

typedef struct {
    u32        idMCU[3];
    u16        idTrain;
    u8         idTrainCar;
    u16        idReceiver;
    u8         idDev;
    u8         idFirmware;
    u8         idNewFirmware;
    u8         szNewFirmware;
    u8         idBoot;
    u8         isSentData;
    u8         isTCPOpen;
    u8         updTarget;
    u8         tcpErrCnt;
    u8         csq;
    u32        gpsInvaligCount;
    u32        gpsParseFailCount;
    TabloInfo  tablo;
    SleepTimer sleepTimer;
} BSG;

typedef struct {
    u32 unixTimeStamp;
    u32 data;
    u8  group;
    u8  code;
} PckgTelemetry;

typedef struct {
    u32 fromByte;
    u32 toByte;
} PckgUpdFirmware;

typedef enum {
    CMD_DATA_VOLTAMPER = 1,
    CMD_DATA_ENERGY,
    CMD_DATA_TEMP,
    CMD_DATA_GRMC,
    CMD_DATA_TELEMETRY
} CMD_DATA_TYPE;

typedef enum {
    TEL_OFF_DEV = 0x0010,
    TEL_ON_DEV = 0x0001,
    TEL_KEEP_ALIVE = 0x0013,
    TEL_ID_FIRMWARE = 0x1001,
    TEL_CHANGE_TIME = 0x1011,
    TEL_BIG_DIFFER_RTC_SERVERTIME = 0x1030,
    TEL_SERV_FLASH_CIRC_BUF_FULL = 0x2001,
    TEL_SERV_FLASH_CIRC_BUF_END_HEAD = 0x2002,
    TEL_SERV_FLASH_CIRC_BUF_END_TAIL = 0x2003,
    TEL_SERV_FLASH_CIRC_BUF_HALF_HEAD = 0x2004,
    TEL_SERV_FLASH_CIRC_BUF_HALF_TAIL = 0x2005,
    TEL_BAD_RESPONSE_SERVER = 0x2006,
    TEL_BAD_RTC_TIME = 0x2007,
    TEL_BAD_ALL_CRC = 0x2008,
    TEL_LORA_LINK_EDGE = 0x2020,
    TEL_LORA_LINK_MASTER = 0x2021,
    TEL_LORA_FLAGS = 0x2022,
    TEL_LORA_BAD_CRC = 0x2023,
    TEL_NO_FATFS = 2030,
    TEL_NO_DS2482 = 2031,
    TEL_PERIPH_STAT = 2032,
    TEL_LVL_CSQ = 0x7010
} TYPE_TELEMETRY;

typedef enum { TEL_GR_GENINF = 1,
               TEL_GR_HARDWARE_STATUS } TELEMETRY_GROUP;

typedef enum {
    TEL_CD_GENINF_NUM_FIRMWARE = 1,
    TEL_CD_GENINF_NUM_BOOT,
    TEL_CD_GENINF_PHONE_NUM1,
    TEL_CD_GENINF_PHONE_NUM2,
    TEL_CD_GENINF_SIMCARD_MONEY,
    TEL_CD_GENINF_NUM_PCB,
    TEL_CD_GENINF_GPS_INV_CNT,
    TEL_CD_GENINF_GPS_PARSE_ER_CNT
} TELEMETRY_CODE_GEN_INF;

typedef enum {
    TEL_CD_HW_BSG = 1,
    TEL_CD_HW_SD,
    TEL_CD_HW_DS2482,
    TEL_CD_HW_SPI_FLASH,
    TEL_CD_HW_LORA,
    TEL_CD_HW_BATTERY,
    TEL_CD_HW_BSG_ALIVE,
    TEL_CD_HW_WIRELESS_SENS_RSSI,
    TEL_CD_HW_UPDATED,
    TEL_CD_HW_UPDATE_LEN,
    TEL_CD_HW_UPDATE_ERR
} TELEMETRY_CODE_STATES;

typedef enum {
    CMD_REQUEST_SERVER_TIME = 0x11,
    CMD_REQUEST_NUM_FIRMWARE,
    CMD_REQUEST_SZ_FIRMWARE,
    CMD_REQUEST_PART_FIRMWARE
} CMD_REQUEST;

typedef enum { MSG_TEMP = 0xF000,
               MSG_TELEMETRY = 0x0000 } TYPE_MSG;

extern BSG      bsg;
extern DateTime dateTimeGprmc;

void bsgInit();

u32 getFlashData(u32 ADDR);

u8   isCrcOk(char* pData, int len);
void getServerTime();

void updSpiFlash(CircularBuffer* cbuf);
u8   waitGoodCsq(u32 timeout);

void saveData(u8* data, u8 sz, u8 cmdData, CircularBuffer* cbuf);
u32  getUnixTimeStamp();
u8   isDataFromFlashOk(char* pData, u8 len);
void copyTelemetry(u8* buf, PckgTelemetry* pckgTel);
void saveTelemetry(PckgTelemetry* pckg, CircularBuffer* cbuf);

void getBsgNumFw();
void getTabloNumFw();

#endif /* INC_UTILS_BSG_H_ */
