/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef  hdma_usart1_rx;
DMA_HandleTypeDef  hdma_usart1_tx;
DMA_HandleTypeDef  hdma_usart2_rx;
DMA_HandleTypeDef  hdma_usart6_rx;
DMA_HandleTypeDef  hdma_usart6_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}
/* USART2 init function */

void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}
/* USART6 init function */

void MX_USART6_UART_Init(void) {
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (uartHandle->Instance == USART1) {
        /* USER CODE BEGIN USART1_MspInit 0 */

        /* USER CODE END USART1_MspInit 0 */
        /* USART1 clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART1 DMA Init */
        /* USART1_RX Init */
        hdma_usart1_rx.Instance = DMA2_Stream2;
        hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart1_rx);

        /* USART1_TX Init */
        hdma_usart1_tx.Instance = DMA2_Stream7;
        hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_tx.Init.Mode = DMA_NORMAL;
        hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart1_tx);

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspInit 1 */

        /* USER CODE END USART1_MspInit 1 */
    } else if (uartHandle->Instance == USART2) {
        /* USER CODE BEGIN USART2_MspInit 0 */

        /* USER CODE END USART2_MspInit 0 */
        /* USART2 clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART2 DMA Init */
        /* USART2_RX Init */
        hdma_usart2_rx.Instance = DMA1_Stream5;
        hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart2_rx);

        /* USART2 interrupt Init */
        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        /* USER CODE BEGIN USART2_MspInit 1 */

        /* USER CODE END USART2_MspInit 1 */
    } else if (uartHandle->Instance == USART6) {
        /* USER CODE BEGIN USART6_MspInit 0 */

        /* USER CODE END USART6_MspInit 0 */
        /* USART6 clock enable */
        __HAL_RCC_USART6_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USART6 DMA Init */
        /* USART6_RX Init */
        hdma_usart6_rx.Instance = DMA2_Stream1;
        hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
        hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart6_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart6_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart6_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart6_rx);

        /* USART6_TX Init */
        hdma_usart6_tx.Instance = DMA2_Stream6;
        hdma_usart6_tx.Init.Channel = DMA_CHANNEL_5;
        hdma_usart6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart6_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart6_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart6_tx.Init.Mode = DMA_NORMAL;
        hdma_usart6_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart6_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart6_tx);

        /* USART6 interrupt Init */
        HAL_NVIC_SetPriority(USART6_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART6_IRQn);
        /* USER CODE BEGIN USART6_MspInit 1 */

        /* USER CODE END USART6_MspInit 1 */
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle) {
    if (uartHandle->Instance == USART1) {
        /* USER CODE BEGIN USART1_MspDeInit 0 */

        /* USER CODE END USART1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART1_CLK_DISABLE();

        /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

        /* USART1 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        /* USART1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspDeInit 1 */

        /* USER CODE END USART1_MspDeInit 1 */
    } else if (uartHandle->Instance == USART2) {
        /* USER CODE BEGIN USART2_MspDeInit 0 */

        /* USER CODE END USART2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART2_CLK_DISABLE();

        /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);

        /* USART2 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmarx);

        /* USART2 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        /* USER CODE BEGIN USART2_MspDeInit 1 */

        /* USER CODE END USART2_MspDeInit 1 */
    } else if (uartHandle->Instance == USART6) {
        /* USER CODE BEGIN USART6_MspDeInit 0 */

        /* USER CODE END USART6_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART6_CLK_DISABLE();

        /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);

        /* USART6 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        /* USART6 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART6_IRQn);
        /* USER CODE BEGIN USART6_MspDeInit 1 */

        /* USER CODE END USART6_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
UartInfo uInfoGnss;
UartInfo uInfoSim;
UartInfo uInfoTablo;

extern osSemaphoreId uart6RXSemHandle;

static u8 usart2BufRx[USART_SZ_BUF_RX_USART2];

static u8 usartSimBufRx[USART_SZ_BUF_RX_USART1];
static u8 usartSimBufTx[USART_SZ_BUF_TX_USART1];

static u8 usartTabloBufRx[USART_SZ_BUF_RX_USART6];
static u8 usartTabloBufTx[USART_SZ_BUF_TX_USART6];

void uartInitInfo() {
    uInfoGnss.irqFlags.regIrq = 0;
    uInfoGnss.pRxBuf = usart2BufRx;
    uInfoGnss.szRxBuf = USART_SZ_BUF_RX_USART2;
    uInfoGnss.pHuart = &huart2;

    uInfoSim.irqFlags.regIrq = 0;
    uInfoSim.pRxBuf = usartSimBufRx;
    uInfoSim.szRxBuf = USART_SZ_BUF_RX_USART1;
    uInfoSim.szTxBuf = USART_SZ_BUF_TX_USART1;
    uInfoSim.pTxBuf = usartSimBufTx;
    uInfoSim.pHuart = &huart1;
    uartRxDma(&uInfoSim);

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    uInfoTablo.irqFlags.regIrq = 0;
    uInfoTablo.pRxBuf = usartTabloBufRx;
    uInfoTablo.szRxBuf = USART_SZ_BUF_RX_USART6;
    uInfoTablo.szTxBuf = USART_SZ_BUF_TX_USART6;
    uInfoTablo.pTxBuf = usartTabloBufTx;
    uInfoTablo.pHuart = &huart6;
    uartRxDma(&uInfoTablo);

    USART_RE6_READ_EN();
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
}

void uartRxDma(UartInfo* pUInf) {
    HAL_UART_Receive_DMA(pUInf->pHuart, pUInf->pRxBuf, pUInf->szRxBuf);
}

void uartClearInfo(UartInfo* pUinf) {
    pUinf->irqFlags.regIrq = 0;
    memset(pUinf->pRxBuf, '\0', pUinf->szRxBuf);
}

void uartTx(char* data, u16 sz, UartInfo* pUInf) {
    uartClearInfo(pUInf);
    __HAL_DMA_DISABLE(pUInf->pHuart->hdmarx);
    __HAL_DMA_SET_COUNTER(pUInf->pHuart->hdmarx, pUInf->szRxBuf);
    __HAL_DMA_ENABLE(pUInf->pHuart->hdmarx);

    waitRx("", &(pUInf->irqFlags), 10, USART_TIMEOUT);
    __HAL_UART_DISABLE_IT(pUInf->pHuart, UART_IT_IDLE);
    osDelay(100);
    pUInf->irqFlags.regIrq = 0;

    HAL_UART_Transmit_DMA(pUInf->pHuart, (u8*)data, sz);

    waitTx("", &(pUInf->irqFlags), 50, USART_TIMEOUT);
}

void uart6Tx(char* data, u16 sz, UartInfo* pUInf) {
    uartClearInfo(pUInf);
    __HAL_DMA_DISABLE(pUInf->pHuart->hdmarx);
    __HAL_DMA_SET_COUNTER(pUInf->pHuart->hdmarx, pUInf->szRxBuf);
    __HAL_DMA_ENABLE(pUInf->pHuart->hdmarx);

    pUInf->irqFlags.regIrq = 0;
    USART_RE6_WRITE_EN();
    __HAL_UART_DISABLE_IT(pUInf->pHuart, UART_IT_IDLE);

    HAL_UART_Transmit(pUInf->pHuart, (u8*)data, sz, 300);

    __HAL_UART_ENABLE_IT(pUInf->pHuart, UART_IT_IDLE);
    USART_RE6_READ_EN();
}

void uartIdleHandler(UART_HandleTypeDef* huart) {
    u16 pkt_len;

    if (huart->Instance == USART6) {
        LOG_USART(LEVEL_DEBUG, "uartIdleHandler Tablo\r\n");
        uInfoTablo.irqFlags.isIrqIdle = 1;

        pkt_len = uInfoTablo.szRxBuf - huart->hdmarx->Instance->NDTR;
        uInfoTablo.pRxBuf[pkt_len] = 0;

        osSemaphoreRelease(uart6RXSemHandle);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* uartHandle) {
    if (uartHandle->Instance == USART1) {
        uInfoSim.irqFlags.isIrqRx = 1;
        uartRxDma(&uInfoSim);
    } else if (uartHandle->Instance == USART2) {
        uInfoGnss.irqFlags.isIrqRx = 1;
        uartRxDma(&uInfoGnss);
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART2) {
        //		printf("GPS: RxHalfCpltCallback\r\n");
        //		printf("GNSS HALFIRQ\r\n");
        uInfoGnss.irqFlags.isIrqRx = 1;
        //		printf("%s", gnssUartInfo.rxBuffer);
        //		test++;
        //		if(test == 2)
        //			printf("test\r\n");
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART1) {
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
        uInfoSim.irqFlags.isIrqTx = 1;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    u32 error = HAL_UART_GetError(huart);
    if (huart->Instance == USART1) {
        LOG_USART(LEVEL_ERROR, "ErrorCallback() sim800 ERROR_CODE: %d\r\n", (int)error);
        uartRxDma(&uInfoSim);
    } else if (huart->Instance == USART2) {
        LOG_USART(LEVEL_ERROR, "ErrorCallback() gnss ERROR_CODE: %d\r\n", (int)error);
        uartRxDma(&uInfoGnss);
    } else if (huart->Instance == USART6) {
        // uartClearInfo(&uInfoTablo);
        __HAL_DMA_DISABLE(uInfoTablo.pHuart->hdmarx);
        __HAL_DMA_SET_COUNTER(uInfoTablo.pHuart->hdmarx, uInfoTablo.szRxBuf);
        __HAL_DMA_ENABLE(uInfoTablo.pHuart->hdmarx);
        uartRxDma(&uInfoTablo);
        LOG_USART(LEVEL_ERROR, "ErrorCallback() tablo ERROR_CODE: %d\r\n", (int)error);
    }
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
