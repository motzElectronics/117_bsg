#include "../Utils/Inc/utils_gps.h"

// Gprmc gprmc;

void setTime(DateTime* dt, char* gpsUbloxTime) {
    u32 time = atoi(gpsUbloxTime);
    dt->sec = time % 100;
    dt->min = (time / 100) % 100;
    dt->hour = (time / 10000) % 100;
}

void setDate(DateTime* dt, char* gpsUbloxDate) {
    u32 date = atoi(gpsUbloxDate);
    dt->year = date % 100;
    dt->month = (date / 100) % 100;
    dt->day = (date / 10000) % 100;
}

// $GNGGA,132948.303,,,,,0,0,,,M,,M,,*53
// $GNRMC,132948.303,V,,,,,0.00,0.00,111121,,,N*55
u8 fillGprmc(char* pData, PckgGnss* pckg) {
    u8    ret = GPS_OK;
    char* token;

    token = strsep(&pData, ",");  // $GPGGA
    if (*token != '$') {
        return GPS_GPRMC_ERR_PARS;
    }
    token = strsep(&pData, ",");  // time
    token = strsep(&pData, ",");  // latitude
    token = strsep(&pData, ",");  // N/S
    token = strsep(&pData, ",");  // longtitude
    token = strsep(&pData, ",");  // W/E
    token = strsep(&pData, ",");  // pos fix indicator

    token = strsep(&pData, ",");  // sattelites
    if (*token != '\0') {
        pckg->coords.sattelites = atoi(token);
    } else {
        return GPS_GPGGA_ERR_PARS_SAT;
    }

    token = strsep(&pData, ",");  // hdop
    if (*token != '\0') {
        pckg->coords.hdop = (int)(atof(token) * 100);
    } else {
        return GPS_GPGGA_ERR_PARS_HDOP;
    }

    token = strsep(&pData, ",");  // altitude
    if (*token != '\0') {
        pckg->coords.altitude = (int)(atof(token) * 100);
    } else {
        return GPS_GPGGA_ERR_PARS_ALT;
    }

    token = strsep(&pData, "\n");

    token = strsep(&pData, ",");
    if (*token != '$') {
        return GPS_GPRMC_ERR_PARS;
    }

    token = strsep(&pData, ",");  // hhmmss.ss
    if (*token != '\0') {
        setTime(&pckg->dateTime, token);
    } else {
        return GPS_GPRMC_ERR_PARS_TIME;
    }
    token = strsep(&pData, ",");  // validity - A-ok, V-invalid
    if (*token == 'A') {
        if ((ret = setCoords(&pData, &pckg->coords.latitude)) != GPS_OK) return ret;
        if ((ret = setCoords(&pData, &pckg->coords.longitude)) != GPS_OK) return ret;

        token = strsep(&pData, ",");  // speed
        if (*token != '\0') {
            pckg->coords.speed = (int)(atof(token) * 1.8 * 10);
        } else {
            return GPS_GPRMC_ERR_PARS_SPEED;
        }

        token = strsep(&pData, ",");  // course
        if (*token != '\0') {
            pckg->coords.course = (int)(atof(token) * 10);
        } else {
            return GPS_GPRMC_ERR_PARS_COURSE;
        }

        token = strsep(&pData, ",");
        if (*token != '\0') {
            setDate(&pckg->dateTime, token);
        } else {
            return GPS_GPRMC_ERR_PARS_DATE;
        }

    } else if (*token == 'V') {
        return GPS_GPRMC_ERR_INVALID_DATA_STATUS;
    } else {
        return GPS_GPRMC_ERR_PARS;
    }
    return ret;
}

char* strsep(char** stringp, const char* delim) {
    char* start = *stringp;
    char* p;
    p = (start != NULL) ? strpbrk(start, delim) : NULL;
    if (p == NULL) {
        *stringp = NULL;
    } else {
        *p = '\0';
        *stringp = p + 1;
    }
    return start;
}

u8 setCoords(char** pData, Coord* coord) {
    u8    ret = GPS_OK;
    char* token;
    char* token1;
    char* savePtr;

    token = strsep(pData, ",");  // coords
    if (*token != '\0') {
        token1 = strtok_r(token, ".", &savePtr);
        if (token1 != NULL) {
            coord->fst = atoi(token1);
            token1 = strtok_r(NULL, ".", &savePtr);
            coord->sec = token1 != NULL ? atoi(token1) : 0;
        }
    } else {
        return GPS_GPRMC_ERR_PARS_COORDS;
    }

    token = strsep(pData, ",");
    if (*token != '\0') {
        // coord->indicator = *token;
    } else {
        return GPS_GPRMC_ERR_PARS_COORDS;
    }

    return ret;
}

void serializePckgGnss(u8* dest, PckgGnss* pckg) {
    pckg->unixTimeStamp = getUnixTimeStamp();

    // LOG_GPS(LEVEL_INFO, "Sat: %d, HDOP: %d, Alt: %d\r\n", pckg->coords.sattelites, pckg->coords.hdop, pckg->coords.altitude);

    memcpy(dest, pckg, SZ_CMD_GEO_PLUS);
}

void deserializePckgGnss(PckgGnss* pckg, u8* source) {
    memcpy(pckg, source, SZ_CMD_GEO_PLUS);
}