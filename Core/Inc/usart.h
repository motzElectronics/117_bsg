/**
 ******************************************************************************
 * @file    usart.h
 * @brief   This file contains all the function prototypes for
 *          the usart.c file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART6_UART_Init(void);

/* USER CODE BEGIN Prototypes */
#include "cmsis_os.h"

#define USART_RE6_WRITE_EN() HAL_GPIO_WritePin(USART6_RE_GPIO_Port, USART6_RE_Pin, GPIO_PIN_SET)
#define USART_RE6_READ_EN()  HAL_GPIO_WritePin(USART6_RE_GPIO_Port, USART6_RE_Pin, GPIO_PIN_RESET)

#define USART_SZ_BUF_RX_USART2 500

#define USART_SZ_BUF_RX_USART1 1524
#define USART_SZ_BUF_TX_USART1 1024

#define USART_SZ_BUF_RX_USART6 2048
#define USART_SZ_BUF_TX_USART6 2048

#define USART_TIMEOUT 15000

typedef UART_HandleTypeDef* PHuart;

typedef struct {
    IrqFlags irqFlags;
    u8*      pRxBuf;
    u8*      pTxBuf;
    u16      szRxBuf;
    u16      szTxBuf;
    PHuart   pHuart;
} UartInfo;

extern UartInfo uInfoGnss;
extern UartInfo uInfoSim;

void setBaudRateUart(UART_HandleTypeDef* huart, u32 baudrate);
void uartTx(char* data, u16 sz, UartInfo* pUInf);
void clearRx(UART_HandleTypeDef* huart);

void rxUart(UartInfo* pUInf);
void uart6Tx(char* data, u16 sz, UartInfo* pUInf);
void rxUart1_IT();
void uartRxDma(UartInfo* pUInf);
void uartClearInfo(UartInfo* pUinf);
void uartInitInfo();
void uartIdleHandler(UART_HandleTypeDef* huart);

// void uartTxLCD(char* data, u16 sz, UartInfo* pUInf);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
