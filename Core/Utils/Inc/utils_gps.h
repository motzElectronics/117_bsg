#ifndef __UTILS_GPS_H
#define __UTILS_GPS_H

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bsg.h"
#include "../Utils/Inc/utils_crc.h"
#include "main.h"

#define GPS_OK 0
#define GPS_GPRMC_ERR_INVALID_DATA_STATUS 1
#define GPS_GPRMC_ERR_PARS_COORDS 2
#define GPS_GPRMC_ERR_PARS_SPEED 3
#define GPS_GPRMC_ERR_PARS_COURSE 4
#define GPS_GPRMC_ERR_PARS_TIME 5
#define GPS_GPRMC_ERR_PARS_DATE 6

typedef struct __attribute__((__packed__)) {
    u32         sec;
    u16         fst;
    // char	indicator;
} Coord;

typedef struct __attribute__((__packed__)) {
    u32         unixTimeStamp;
    Coord       latitude;
    Coord       longitude;
    u16         course;
    u16         speed;
    DateTime    dateTime;
} PckgGnss;

u8 fillGprmc(char* pData, PckgGnss* pckg);
void clearGprmc();
u8 setCoords(char** pData, Coord* coord);
char* strsep(char** stringp, const char* delim);
void serializePckgGnss(u8* dest, PckgGnss* pckg);
void deserializePckgGnss(PckgGnss* pckg, u8* source);
#endif