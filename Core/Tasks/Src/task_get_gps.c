#include "../Tasks/Inc/task_get_gps.h"
extern osMutexId mutexGPSBufHandle;
extern CircularBuffer circBufPckgGPS;
extern osThreadId httpHandle;
static char bufGnss[200];
static PckgGnss pckgGnss;
static u8 bufPckgGnss[GPS_SZ_GRMC];
CircularBuffer circBufGnss = {.buf = NULL, .max = 0};
void taskGetGPS(void const * argument)
{
	cBufInit(&circBufGnss, uInfoGnss.pRxBuf, uInfoGnss.szRxBuf, CIRC_TYPE_GNSS);

	spiFlashInit(circBufPckgGPS.buf);
	simInit();
	if(!getServerTime()){
		D(printf("ERROR: BAD TIME\r\n"));
	}
	fillTelemetry(TEL_ON_DEV, 0);
	fillTelemetry(TEL_ID_FIRMWARE, bsg.idFirmware);
	vTaskResume(httpHandle);

	for(;;){
		waitRx("", &uInfoGnss.irqFlags, 1000, 10000);
		if(uInfoGnss.irqFlags.isIrqRx){
			uInfoGnss.irqFlags.isIrqRx = 0;
			while(cBufRead(&circBufGnss, (u8*)bufGnss, CIRC_TYPE_GNSS, 0)){
				D(printf("GPS: %s", bufGnss));
				if(fillGprmc(bufGnss, &pckgGnss) == GPS_OK){
					serializePckgGnss(bufPckgGnss, &pckgGnss);
					writeSafeToCBuf(&circBufPckgGPS, bufPckgGnss, GPS_SZ_GRMC, mutexGPSBufHandle);
				}
				memset(bufGnss, '\0', sizeof(bufGnss));
			}
			D(printf("empty circBufGnss\r\n"));
		} else{
			D(printf("ERROR: NOT WORKING GPS\r\n"));
		}
	}
  /* USER CODE END taskGetGPS */
}