/*
 * utils_json.h
 *
 *  Created on: Jun 12, 2020
 *      Author: annsi
 */

#ifndef INC_UTILS_JSON_H_
#define INC_UTILS_JSON_H_
#include "main.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/utils_gps.h"
//#define JSON_SZ_GNSS	1000
#define JSON_NUM_GNSS_PCKG_IN_ONE_TX	3

typedef struct{
	u16 numPckg;
	char jsonGnssTxBuf[SZ_MAX_TX_DATA];

}PckgJsonGnss;

void addGnssPckgToJson(PckgGnss* pPckgGnss);
u16 closeGnssJson();
void pckgUpdFirmwareToJson(PckgUpdFirmware* source, char* dist, u16 szDist);

#endif /* INC_UTILS_JSON_H_ */
