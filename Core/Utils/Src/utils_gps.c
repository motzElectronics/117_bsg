#include "../Utils/Inc/utils_gps.h"

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
    char* p_gngga;
    char* p_gnrmc;

    // Check GNGGA, GNRMC prefixes
    p_gngga = strsep(&pData, "\n");
    p_gnrmc = strsep(&pData, "\n");

    token = strsep(&p_gngga, ",");  // $GNGGA
    if (strcmp(token, "$GNGGA") != 0) {
        return GPS_GPGGA_ERR_PARS;
    }
    token = strsep(&p_gnrmc, ",");  // $GNRMC
    if (strcmp(token, "$GNRMC") != 0) {
        return GPS_GPRMC_ERR_PARS;
    }

    // Parse GNRMC string
    token = strsep(&p_gnrmc, ",");  // hhmmss.ss
    if (*token != '\0') {
        setTime(&pckg->dateTime, token);
    } else {
        return GPS_GPRMC_ERR_PARS_TIME;
    }
    token = strsep(&p_gnrmc, ",");  // validity - A-ok, V-invalid
    if (*token == 'A') {
        if ((ret = setCoords(&p_gnrmc, &pckg->coords.latitude)) != GPS_OK) return ret;
        if ((ret = setCoords(&p_gnrmc, &pckg->coords.longitude)) != GPS_OK) return ret;

        token = strsep(&p_gnrmc, ",");  // speed
        if (*token != '\0') {
            pckg->coords.speed = (int)(atof(token) * 1.8 * 10);
        } else {
            return GPS_GPRMC_ERR_PARS_SPEED;
        }
        token = strsep(&p_gnrmc, ",");  // course
        if (*token != '\0') {
            pckg->coords.course = (int)(atof(token) * 10);
        } else {
            return GPS_GPRMC_ERR_PARS_COURSE;
        }
        token = strsep(&p_gnrmc, ",");
        if (*token != '\0') {
            setDate(&pckg->dateTime, token);
        } else {
            return GPS_GPRMC_ERR_PARS_DATE;
        }

        // Parse GNGGA string
        token = strsep(&p_gngga, ",");  // time
        token = strsep(&p_gngga, ",");  // latitude
        token = strsep(&p_gngga, ",");  // N/S
        token = strsep(&p_gngga, ",");  // longtitude
        token = strsep(&p_gngga, ",");  // W/E
        token = strsep(&p_gngga, ",");  // pos fix indicator

        token = strsep(&p_gngga, ",");  // sattelites
        if (*token != '\0') {
            pckg->coords.sattelites = atoi(token);
        } else {
            return GPS_GPGGA_ERR_PARS_SAT;
        }
        token = strsep(&p_gngga, ",");  // hdop
        if (*token != '\0') {
            pckg->coords.hdop = (int)(atof(token) * 100);
        } else {
            return GPS_GPGGA_ERR_PARS_HDOP;
        }
        token = strsep(&p_gngga, ",");  // altitude
        if (*token != '\0') {
            pckg->coords.altitude = (int)(atof(token) * 100);
        } else {
            return GPS_GPGGA_ERR_PARS_ALT;
        }
    } else if (*token == 'V') {
        return GPS_GPRMC_ERR_INVALID_DATA_STATUS;
    } else {
        return GPS_GPRMC_ERR_PARS_FLAG;
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