#ifndef __DEBUG_H
#define __DEBUG_H

#include "main.h"

#define DEBUG_USART 1
#define DEBUG_PPRU  1
#define DEBUG_PARAM 1
#define DEBUG_CBUF  1
#define DEBUG_FLASH 1

#define LEVEL_ERROR 1
#define LEVEL_MAIN  2
#define LEVEL_INFO  3
#define LEVEL_DEBUG 4

#define DEBUG_LEVEL LEVEL_DEBUG
// #define DEBUG_LEVEL     LEVEL_INFO
// #define DEBUG_LEVEL     LEVEL_MAIN
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL LEVEL_ERROR
#endif

#define PRINT_LEVEL(level)                 \
    do {                                   \
        if (level == LEVEL_ERROR) {        \
            printf("[error]:");            \
        } else if (level == LEVEL_MAIN) {  \
            printf("[main] :");            \
        } else if (level == LEVEL_INFO) {  \
            printf("[info] :");            \
        } else if (level == LEVEL_DEBUG) { \
            printf("[debug]:");            \
        }                                  \
    } while (0)

#define LOG(level, ...)             \
    do {                            \
        if (DEBUG_LEVEL >= level) { \
            PRINT_LEVEL(level);     \
            printf("Common:");      \
            printf(__VA_ARGS__);    \
        }                           \
    } while (0)

#ifdef DEBUG_USART
#define LOG_USART(level, ...)       \
    do {                            \
        if (DEBUG_LEVEL >= level) { \
            PRINT_LEVEL(level);     \
            printf("USART :");      \
            printf(__VA_ARGS__);    \
        }                           \
    } while (0)
#else
#define LOG_USART(level, ...)
#endif

#ifdef DEBUG_PPRU
#define LOG_PPRU(level, ...)        \
    do {                            \
        if (DEBUG_LEVEL >= level) { \
            PRINT_LEVEL(level);     \
            printf("PPRU  : ");     \
            printf(__VA_ARGS__);    \
        }                           \
    } while (0)
#else
#define LOG_PPRU(level, ...)
#endif

#ifdef DEBUG_PARAM
#define LOG_PARAM(level, ...)       \
    do {                            \
        if (DEBUG_LEVEL >= level) { \
            PRINT_LEVEL(level);     \
            printf("PARAM :");      \
            printf(__VA_ARGS__);    \
        }                           \
    } while (0)
#else
#define LOG_PARAM(level, ...)
#endif

#ifdef DEBUG_CBUF
#define LOG_CBUF(level, ...)        \
    do {                            \
        if (DEBUG_LEVEL >= level) { \
            PRINT_LEVEL(level);     \
            printf("CBUF  :");      \
            printf(__VA_ARGS__);    \
        }                           \
    } while (0)
#else
#define LOG_CBUF(level, ...)
#endif

#ifdef DEBUG_FLASH
#define LOG_FLASH(level, ...)       \
    do {                            \
        if (DEBUG_LEVEL >= level) { \
            PRINT_LEVEL(level);     \
            printf("FLASH :");      \
            printf(__VA_ARGS__);    \
        }                           \
    } while (0)
#else
#define LOG_FLASH(level, ...)
#endif

#endif