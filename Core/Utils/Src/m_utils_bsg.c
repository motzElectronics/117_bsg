#include "../Utils/Inc/utils_bsg.h"
#include "rtc.h"
#include "../Utils/Inc/utils_gps.h"
extern BSG bsg;
extern osMutexId mutexGPSBufHandle;
extern CircularBuffer circBufTTLVtoFlash;
extern CircularBuffer circBufPckgGPS;
extern osMutexId mutexRTCHandle;
static u16 bufEnd[2] = {0, BSG_PREAMBLE_SYN};
static RTC_TimeTypeDef tmpTime;
static RTC_DateTypeDef tmpDate;
void bsgInit(){

	for(u8 i = 0; i < 3; i++)
		bsg.idMCU[i] = getFlashData(BSG_ADDR_ID_MCU + (i * 4));
	D(printf("%08x%08x%08x\r\n",
					(uint)bsg.idMCU[0], (uint)bsg.idMCU[1], (uint)bsg.idMCU[2]));

	bsg.idTrain = BKTE_ID_TRAIN;
	bsg.idTrainCar = BSG_ID_TRAINCAR;
	bsg.idReceiver = 1;
	bsg.idDev = BSG_ID_DEV_BSG;
	bsg.idFirmware = BSG_ID_FIRMWARE;
	bsg.idBoot = BSG_ID_BOOT;
}



u32 getFlashData(u32 ADDR){
	return (*(__IO u32*) ADDR);
}

void checkBufForWritingToFlash(CBufHandle pCircBuf,u8 sz){
	if(pCircBuf->writeAvailable < sz + 4){
		bufEnd[0] = calcCrc16(pCircBuf->buf, pCircBuf->readAvailable);
		cBufWriteToBuf(pCircBuf, (u8*)bufEnd, sizeof(bufEnd));
		spiFlashWrPg(pCircBuf->buf, pCircBuf->readAvailable, 0, spiFlash64.headNumPg);
		cBufReset(pCircBuf);
	}
}

void writeSafeToCBuf(CBufHandle cbuf, u8* data, u8 sz, osMutexId mutex){
	xSemaphoreTake(mutex, portMAX_DELAY);
	checkBufForWritingToFlash(cbuf, sz);
	cBufWriteToBuf(cbuf, data, sz);
	xSemaphoreGive(mutex);
}

u32 getServerTime(){

	u8 tmpSimBadResponse = 0;
	char timestamp[LEN_TIMESTAMP + 1];
	memset(timestamp, '\0', sizeof(timestamp));
	D(printf("getServerTime()\r\n"));
	while(simGetDateTime(timestamp) != SIM_SUCCESS){
		memset(timestamp, '\0', sizeof(timestamp));
		D(printf("ERROR: bad time\r\n"));
//		HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_RESET);
		tmpSimBadResponse = (tmpSimBadResponse + 1) % 10;
		if(tmpSimBadResponse % 4 == 0){
			simReset();
			return 0;
		} else if(!tmpSimBadResponse){
			HAL_NVIC_SystemReset();
		}
		osDelay(1000);
//		rxUartSIM_IT();
	}
	timestamp[LEN_TIMESTAMP] = '\0';
	time_t t = strtoull(timestamp, NULL, 0);
    struct tm* pTm;
	pTm = gmtime(&t);
	if(pTm != NULL){
		tmpTime.Hours = pTm->tm_hour;
		tmpTime.Minutes = pTm->tm_min;
		tmpTime.Seconds = pTm->tm_sec;

		tmpDate.Date = pTm->tm_mday;
		tmpDate.Month = pTm->tm_mon;
		tmpDate.Year = pTm->tm_year - 100;

		if(tmpDate.Year < 30 && tmpDate.Year > 19){  //sometimes timestamp is wrong and has value like 2066 year
			HAL_RTC_SetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
		}
	}
	return t;
}


void setDateTime(DateTime* dt){
//	RTC_TimeTypeDef tmpTime;
//	RTC_DateTypeDef tmpDate;
	xSemaphoreTake(mutexRTCHandle, portMAX_DELAY);
	HAL_RTC_GetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
	dt->day = tmpDate.Date;
	dt->month = tmpDate.Month;
	dt->year = tmpDate.Year;
	dt->sec = tmpTime.Seconds;
	dt->min = tmpTime.Minutes;
	dt->hour = tmpTime.Hours;
	xSemaphoreGive(mutexRTCHandle);
}

time_t getTimeStamp(){
	DateTime tmpDateTime;
	time_t t;
	setDateTime(&tmpDateTime);
	setTM(&t, &tmpDateTime);
	return t;
}

void setTM(time_t* pTimeStamp, DateTime* dt){
	static struct tm curTime;
	curTime.tm_year = dt->year + 100;  // In fact: 2000 + 18 - 1900
	curTime.tm_mday = dt->day;
	curTime.tm_mon  = dt->month;

	curTime.tm_hour = dt->hour;
	curTime.tm_min  = dt->min;
	curTime.tm_sec  = dt->sec;

	curTime.tm_isdst = 0;

	*pTimeStamp = mktime(&curTime);
}

void fillTelemetry(TYPE_TELEMETRY typeTel, u32 val1){ //can add another vals
	PckgGnss pckg;
	u8 buf[GPS_SZ_GRMC];
	pckg.latitude.fst = MSG_TELEMETRY;
	pckg.latitude.indicator = 'N';
	pckg.latitude.sec = 0;
	pckg.longitude.indicator = 'E';
	pckg.longitude.fst = 0;
	pckg.longitude.sec = val1;
	pckg.speed = typeTel;
	pckg.cource = 0;
	setDateTime(&pckg.dateTime);

	serializePckgGnss(buf, &pckg);
	writeSafeToCBuf(&circBufPckgGPS, buf, GPS_SZ_GRMC, mutexGPSBufHandle);
}

u8 isCrcOk(char* pData, int len){
	char tmp[] = {'\0', '\0', '\0'};
	memcpy(tmp, pData + 8, 2);
	u8 crcCalc = crc8(pData + 20, len);
	u8 crcRecv = strtoul(tmp, NULL, 16);
	if(crcCalc == crcRecv){
		return 1;
	} else
		return 0;
}

/*void setTime(DateTime* dt, char* gpsUbloxTime){
	u32 time = atoi(gpsUbloxTime);
	dt->sec = time % 100;
	dt->min = (time / 100) % 100;
	dt->hour = (time / 10000) % 100;
}

void setDate(DateTime* dt, char* gpsUbloxDate){
	u32 date = atoi(gpsUbloxDate);
	dt->year = date % 100;
	dt->month = (date / 100) % 100 - 1;
	dt->day = (date / 10000) % 100;
}

void setTM(u32* pTimeStamp, DateTime* dt){
	static struct tm curTime;
	curTime.tm_year = dt->year + 100;  // In fact: 2000 + 18 - 1900
	curTime.tm_mday = dt->day;
	curTime.tm_mon  = dt->month;

	curTime.tm_hour = dt->hour;
	curTime.tm_min  = dt->min;
	curTime.tm_sec  = dt->sec;

	curTime.tm_isdst = 0;

	*pTimeStamp = (u32)mktime(&curTime);
}*/

// void checkBufForWritingToFlash(u8 sz){
// 	u8 isFullSpiFlash = 0;
// 	if(circBufTTLVtoFlash.writeAvailable < sz + 4){
// 		bufEnd[0] = calcCrc16(circBufTTLVtoFlash.buf, circBufTTLVtoFlash.readAvailable);
// 		cBufWriteToBuf(&circBufTTLVtoFlash, (u8*)bufEnd, sizeof(bufEnd));
// 		isFullSpiFlash =  spiFlashWrPg(circBufTTLVtoFlash.buf, circBufTTLVtoFlash.readAvailable, 0, spiFlash64.headNumPg);
// 		cBufReset(&circBufTTLVtoFlash);
// 		/*if(isFullSpiFlash){
// 		  	fillTelemetry(&curPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_FULL, 0);
// 			cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
// 		}	

// 	  	if(!spiFlash64.headNumPg){
// 			fillTelemetry(&curPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_END_HEAD, 0);
// 			cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
// 	  	} else if(spiFlash64.headNumPg == SPIFLASH_NUM_PG_GNSS / 2){
// 			fillTelemetry(&curPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_HALF_HEAD, 0);
// 			cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
// 		}*/

// 	}
// }