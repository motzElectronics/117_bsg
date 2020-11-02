#include "../Tasks/Inc/task_http.h"
extern osThreadId httpHandle;
extern PckgJsonGnss pckgJsonGnss;
extern osMutexId mutexWebHandle;
extern osThreadId getNewBinHandle;
static u8 CNT_MAX_PAGES_TX;
u8 tmpBufPage[256];
static PckgGnss pckgGnss;
void taskHttp(void const * argument){

    
	s16 delayPages;
	u16 CNT_CUR_PAGES_TX;
	u16 tmpCntPckg = 0;
	u32 notTxCntPckg = 0;
	u32 allTxPckg = 0;
	u32 tmpTimeStamp;
	u8 len;
	vTaskSuspend(httpHandle);


	CNT_MAX_PAGES_TX = SZ_MAX_TX_DATA / spiFlash64.pgSz / 2 - 1;
	simHttpInit(urls.addMeasure);

	for(;;){
		while(spiFlash64.headNumPg != spiFlash64.tailNumPg){
			CNT_CUR_PAGES_TX = getReadyTxPages();
			for(u8 i = 0; i < CNT_CUR_PAGES_TX; i++){
				
				spiFlashRdPg((u8*)tmpBufPage, 256, 0, spiFlash64.tailNumPg);
				if((len = isDataFromFlashOk(tmpBufPage, 256)) && len % GPS_SZ_GRMC == 0){
					D(printf("OK: CRC\r\n"));
					for(u8 i = 0; i < len; i += GPS_SZ_GRMC){
						deserializePckgGnss(&pckgGnss, tmpBufPage + i);
						addGnssPckgToJson(&pckgGnss);
					}

				}
			}
			if(pckgJsonGnss.numPckg){
				tmpCntPckg = closeGnssJson();
				allTxPckg += tmpCntPckg;
				if(sendDataToServer() == PCKG_WAS_lOST){
					notTxCntPckg += tmpCntPckg;
					fillTelemetry(TEL_BAD_RESPONSE_SERVER, (u32)((float)notTxCntPckg / allTxPckg * 1000));
				}
			} else {
				fillTelemetry(TEL_BAD_ALL_CRC, 0);
			}
		}
		D(printf("no pckg in spiflash\r\n"));
		osDelay(3000);
	}

}

u8 sendDataToServer(){
	u8 resCode;
	char* pRxData;
	u8 cntHttpPostFail = 1;
	u8 tmpIdFirmware;
	u8 csq = 0;
	u8 ret = PCKG_WAS_SENT;

	xSemaphoreTake(mutexWebHandle, portMAX_DELAY);

	while(cntHttpPostFail){

		while((csq = simCheckCSQ()) < 15 && csq > 99){
			osDelay(2000);
			saveCsq(csq);
		}
		
		if((resCode = httpPost(pckgJsonGnss.jsonGnssTxBuf, strlen(pckgJsonGnss.jsonGnssTxBuf), &pRxData, 
		10, 10000)) != SIM_SUCCESS){
			D(printf("ERROR: httpPost()\r\n"));
			
			cntHttpPostFail++;
			if(cntHttpPostFail == 3){
				simReset();
			}else if(cntHttpPostFail == 4){
				cntHttpPostFail = 0;
				simReset();
				ret = PCKG_WAS_lOST;
			}
			simHttpInit(urls.addMeasure);	
		} else{
			cntHttpPostFail = 0;
			D(printf("OK: httpPost()\r\n"));
			if((tmpIdFirmware = atoi(pRxData + 11)) != bsg.idFirmware && tmpIdFirmware > 0){
				D(printf("New FIRMWARE v.:%d\r\n", (int)tmpIdFirmware));
				vTaskResume(getNewBinHandle);
			}
		}

	}
	xSemaphoreGive(mutexWebHandle);
	return ret;
}

u8 getReadyTxPages(){
    s16 delayPages;
    u8 cntPages;
    if(spiFlash64.headNumPg > spiFlash64.tailNumPg)
        delayPages = spiFlash64.headNumPg - spiFlash64.tailNumPg;
    else
        delayPages = spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
    cntPages = delayPages > CNT_MAX_PAGES_TX ? CNT_MAX_PAGES_TX : delayPages;
    return cntPages;
}

u8 isDataFromFlashOk(char* pData, u8 len){
    u16 crc;
    for(u8 i = len - 1; i; --i){
        if(pData[i] == BSG_PREAMBLE_LSB && pData[i - 1] == BSG_PREAMBLE_MSB){
            crc = pData[i - 3] | pData[i - 2] << 8;
            if(calcCrc16(pData, i - 3) == crc){
				len = (i + 1) - 4;
                return len;
            }
        }
    }
    return 0;
}

void saveCsq(u8 csq){
	fillTelemetry(TEL_LVL_CSQ, csq);
}