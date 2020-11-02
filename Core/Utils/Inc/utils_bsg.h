/*
 * utils_bsg.h
 *
 *  Created on: Apr 20, 2020
 *      Author: bls
 */

#ifndef INC_UTILS_BSG_H_
#define INC_UTILS_BSG_H_
#include "main.h"
#include "time.h"
#include "../xDrvrs/Inc/spiflash.h"
#include "usart.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_crc.h"
#include "../xDrvrs/Inc/simcom.h"



#define BSG_ID_DEV_BSG		0x12
#define BSG_ADDR_ID_MCU		0x1FFF7A10

#define SZ_PART_NEW_SOFTWARE	1360
#define SZ_MAX_TX_DATA			4096

#define BSG_MSG_NO_GPS			(char*)"0000.000000,N,00000.000000,E,+0000,000,000"

#define BSG_ID_FIRMWARE			1
#define BSG_ID_BOOT				1
#define BSG_ID_TRAINCAR		0
#define BKTE_ID_TRAIN			1706
#define BSG_VER_BETA_FIRMWARE		(char)'B'
#define BSG_VER_STABLE_FIRMWARE	(char)'S'

#define BSG_PREAMBLE_SYN        0xAABB
#define BSG_PREAMBLE_LSB		0xAA
#define BSG_PREAMBLE_MSB		0xBB
#define BSG_CNT_INVALID_GNSS_DATA	5

#define BSG_SZ_BUF_PCKG_GNSS	512

#define BSG_SAVE_NUM_PAGE				2047

#define LEN_TIMESTAMP		13

#define BSG_BIG_DIF_RTC_SERVTIME		600


/*typedef struct{
	DateTime dateTime;
	char msgGPS[SIM_SZ_GPSINFO];
}PckgGPS;

typedef struct{
	u32		sec;
	u16 	fst;
	char	indicator;
}Coord;

typedef struct{
	u16			preambule;
	Coord		latitude;
	Coord		longitude;
	u16			speed;
	u16			cource;
	DateTime	dateTime;
	u8			crc;
}PckgGnss;

typedef struct{
	u32 		preambule;
	u32 		latitudeSec;
	u32 		longitudeSec;
	u16 		latitudeFst;
	u16 		longitudeFst;
	u16 		speed;
	u16			cource;
	DateTime	dateTime;
	char		indicatorLat;
	char		indicatorLong;
	u32			crc;
}PckgGnssTest;

typedef enum{
	NUM_FILE_GPS = 0,
}NUM_FILE;

typedef struct{
	u32 posFile;
	u32 lenLog;
	char* fNameLog;
	char* fNameAddr;
}FInfo;*/



typedef struct{
	u32	idMCU[3];
	u16	idTrain;
	u8	idTrainCar;
	u16	idReceiver;
	u8	idDev;
	u8	idFirmware;
	u8	idBoot;
//	FInfo	fInfo[NUM_READ_FILES];
}BSG;

typedef struct{
	u32 fromByte;
	u32 toByte;
}PckgUpdFirmware;


typedef enum{
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
	TEL_LVL_CSQ = 0x7010
}TYPE_TELEMETRY;


typedef enum{
	MSG_TEMP = 0xF000,
	MSG_TELEMETRY = 0x0000
}TYPE_MSG;



extern BSG bsg;
extern DateTime dateTimeGprmc;

/*u8 fillPckgGnss(PckgGnss* pckg, u8 szPckg, char* data);
//void fillPckgNoGPS(PckgGPS* pckg);
//void fillPckgGPSUblox(PckgGPS* pckg, char* data);
void setDateTime(DateTime* dt, char* gpsData);
void setTM(time_t* pTimeStamp, DateTime* dt);*/
void  bsgInit();
u32 getFlashData(u32 ADDR);
void setTime(DateTime* dt, char* gpsUbloxTime);
void setDate(DateTime* dt, char* gpsUbloxDate);
void setTM(u32* pTimeStamp, DateTime* dt);
/*u8 crc8(char *pcBlock, int len);

u8 isCrcOk(char* pData, int len);
void setTime(DateTime* dt, char* gpsUbloxTime);
void setDate(DateTime* dt, char* gpsUbloxDate);
u8 getGnssPckg(u8* pBuf, u16 szBuf, PckgGnss* pPckgGnss, u8 szPckg);*/

void gnssInit();
void checkBufForWritingToFlash(CBufHandle pCircBuf,u8 sz);
void writeSafeToCBuf(CBufHandle pCircBuf, u8* data, u8 sz, osMutexId mutex);
u32 getServerTime();
void fillTelemetry(TYPE_TELEMETRY typeTel, u32 v1);

time_t getTimeStamp();
u8 isCrcOk(char* pData, int len);
void setDateTime(DateTime* dt);

#endif /* INC_UTILS_BSG_H_ */
