#include "../Utils/Inc/utils_gps.h"

// Gprmc gprmc;

void setTime(DateTime* dt, char* gpsUbloxTime){
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

u8 fillGprmc(char* pData, PckgGnss* pckg){
	u8 ret = GPS_OK;
	char* token;
	char* savePtr;

	token = strsep(&pData, ","); //$GPRMC,
	token = strsep(&pData, ","); // hhmmss.ss
	if(*token != '\0')
		setTime(&pckg->dateTime, token);
	else 
		return GPS_GPRMC_ERR_PARS_TIME;
	token = strsep(&pData, ","); // validity - A-ok, V-invalid
	if(*token == 'A'){
		if((ret = setCoords(&pData, &pckg->latitude)) != GPS_OK)
			return ret;
		if((ret = setCoords(&pData, &pckg->longitude)) != GPS_OK)
			return ret;

		token = strsep(&pData, ","); // speed
		if(*token != '\0'){
			pckg->speed = (int)(atof(token) * 10);
		} else{
			return GPS_GPRMC_ERR_PARS_SPEED;
		}

		token = strsep(&pData, ","); // cource
		if(*token != '\0'){
			pckg->cource = (int)(atof(token) * 10);
		} else{
			return GPS_GPRMC_ERR_PARS_COURSE;
		}
		
		token = strsep(&pData, ",");
		if(*token != '\0')
			setDate(&pckg->dateTime, token);
		else 
			return GPS_GPRMC_ERR_PARS_DATE;

	} else 
		return GPS_GPRMC_ERR_INVALID_DATA_STATUS;
	return ret;
}

char* strsep(char** stringp, const char* delim){
  char* start = *stringp;
  char* p;
  p = (start != NULL) ? strpbrk(start, delim) : NULL;
  if (p == NULL){
    *stringp = NULL;
  }else{
    *p = '\0';
    *stringp = p + 1;
  }
  return start;
}

u8 setCoords(char** pData, Coord* coord){
	u8 ret = GPS_OK;
	char* token;
	char* token1;
	char* savePtr;

	token = strsep(pData, ","); // coords
	if(*token != '\0'){
		token1 = strtok_r(token, ".", &savePtr);
		if(token1 != NULL){
			coord->fst = atoi(token1);
			token1 = strtok_r(NULL, ".", &savePtr);
			coord->sec = token1 != NULL ? atoi(token1) : 0;
		}
	} else {
		return GPS_GPRMC_ERR_PARS_COORDS;
	}

	token = strsep(pData, ","); 
	if(*token != '\0'){
		coord->indicator = *token;
	}else{
		return GPS_GPRMC_ERR_PARS_COORDS;
	}

	return ret;
}

void serializePckgGnss(u8* dest, PckgGnss* pckg){
	memcpy(dest, &pckg->latitude, 7);
	memcpy(dest + 7, &pckg->longitude, 7);
	memcpy(dest + 14, &pckg->speed, 2);
	memcpy(dest + 16, &pckg->cource, 2);
	memcpy(dest + 18, &pckg->dateTime, 6);
}

void deserializePckgGnss(PckgGnss* pckg, u8* source){
	memcpy(&pckg->latitude, source, 7);
	memcpy(&pckg->longitude, source + 7, 7);
	memcpy(&pckg->speed, source + 14, 2);
	memcpy(&pckg->cource, source + 16, 2);
	memcpy(&pckg->dateTime, source + 18, 6);
}