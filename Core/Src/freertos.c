/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"

#include "cmsis_os.h"
#include "main.h"
#include "task.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
#include "../Utils/Inc/utils_pckgs_manager.h"
#include "usart.h"

/* USER CODE END Variables */
osThreadId    getGPSHandle;
osThreadId    keepAliveHandle;
osThreadId    getNewBinHandle;
osThreadId    manageIWDGHandle;
osThreadId    createWebPckgHandle;
osThreadId    webExchangeHandle;
osThreadId    getTrainDataHandle;
osThreadId    sendTelemetryHandle;
osMessageQId  queueWebPckgHandle;
osMessageQId  queueTelPckgHandle;
osMutexId     mutexGPSBufHandle;
osMutexId     mutexWebHandle;
osMutexId     mutexRTCHandle;
osMutexId     mutexSpiFlashHandle;
osMutexId     mutexSessionHandle;
osSemaphoreId semCreateWebPckgHandle;
osSemaphoreId uart6RXSemHandle;
osSemaphoreId semSendWebPckgHandle;
osSemaphoreId semSendTelPckgHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void taskGetGPS(void const *argument);
void taskKeepAlive(void const *argument);
void taskGetNewBin(void const *argument);
void taskManageIWDG(void const *argument);
void taskCreateWebPckg(void const *argument);
void taskWebExchange(void const *argument);
void taskGetTrainData(void const *argument);
void taskSendTelemetry(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */
    /* Create the mutex(es) */
    /* definition and creation of mutexGPSBuf */
    osMutexDef(mutexGPSBuf);
    mutexGPSBufHandle = osMutexCreate(osMutex(mutexGPSBuf));

    /* definition and creation of mutexWeb */
    osMutexDef(mutexWeb);
    mutexWebHandle = osMutexCreate(osMutex(mutexWeb));

    /* definition and creation of mutexRTC */
    osMutexDef(mutexRTC);
    mutexRTCHandle = osMutexCreate(osMutex(mutexRTC));

    /* definition and creation of mutexSpiFlash */
    osMutexDef(mutexSpiFlash);
    mutexSpiFlashHandle = osMutexCreate(osMutex(mutexSpiFlash));

    /* definition and creation of mutexSession */
    osMutexDef(mutexSession);
    mutexSessionHandle = osMutexCreate(osMutex(mutexSession));

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* Create the semaphores(s) */
    /* definition and creation of semCreateWebPckg */
    osSemaphoreDef(semCreateWebPckg);
    semCreateWebPckgHandle = osSemaphoreCreate(osSemaphore(semCreateWebPckg), 1);

    /* definition and creation of uart6RXSem */
    osSemaphoreDef(uart6RXSem);
    uart6RXSemHandle = osSemaphoreCreate(osSemaphore(uart6RXSem), 1);

    /* definition and creation of semSendWebPckg */
    osSemaphoreDef(semSendWebPckg);
    semSendWebPckgHandle = osSemaphoreCreate(osSemaphore(semSendWebPckg), 1);

    /* definition and creation of semSendTelPckg */
    osSemaphoreDef(semSendTelPckg);
    semSendTelPckgHandle = osSemaphoreCreate(osSemaphore(semSendTelPckg), 1);

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* Create the queue(s) */
    /* definition and creation of queueWebPckg */
    osMessageQDef(queueWebPckg, 8, WebPckg *);
    queueWebPckgHandle = osMessageCreate(osMessageQ(queueWebPckg), NULL);

    /* definition and creation of queueTelPckg */
    osMessageQDef(queueTelPckg, 8, WebPckg *);
    queueTelPckgHandle = osMessageCreate(osMessageQ(queueTelPckg), NULL);

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* definition and creation of getGPS */
    osThreadDef(getGPS, taskGetGPS, osPriorityNormal, 0, 300);
    getGPSHandle = osThreadCreate(osThread(getGPS), NULL);

    /* definition and creation of keepAlive */
    osThreadDef(keepAlive, taskKeepAlive, osPriorityNormal, 0, 300);
    keepAliveHandle = osThreadCreate(osThread(keepAlive), NULL);

    /* definition and creation of getNewBin */
    osThreadDef(getNewBin, taskGetNewBin, osPriorityNormal, 0, 300);
    getNewBinHandle = osThreadCreate(osThread(getNewBin), NULL);

    /* definition and creation of manageIWDG */
    osThreadDef(manageIWDG, taskManageIWDG, osPriorityNormal, 0, 128);
    manageIWDGHandle = osThreadCreate(osThread(manageIWDG), NULL);

    /* definition and creation of createWebPckg */
    osThreadDef(createWebPckg, taskCreateWebPckg, osPriorityNormal, 0, 300);
    createWebPckgHandle = osThreadCreate(osThread(createWebPckg), NULL);

    /* definition and creation of webExchange */
    osThreadDef(webExchange, taskWebExchange, osPriorityNormal, 0, 300);
    webExchangeHandle = osThreadCreate(osThread(webExchange), NULL);

    /* definition and creation of getTrainData */
    osThreadDef(getTrainData, taskGetTrainData, osPriorityNormal, 0, 256);
    getTrainDataHandle = osThreadCreate(osThread(getTrainData), NULL);

    /* definition and creation of sendTelemetry */
    osThreadDef(sendTelemetry, taskSendTelemetry, osPriorityNormal, 0, 256);
    sendTelemetryHandle = osThreadCreate(osThread(sendTelemetry), NULL);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_taskGetGPS */
/**
 * @brief  Function implementing the getGPS thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskGetGPS */
__weak void taskGetGPS(void const *argument) {
    /* USER CODE BEGIN taskGetGPS */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskGetGPS */
}

/* USER CODE BEGIN Header_taskKeepAlive */
/**
 * @brief Function implementing the keepAlive thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskKeepAlive */
__weak void taskKeepAlive(void const *argument) {
    /* USER CODE BEGIN taskKeepAlive */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskKeepAlive */
}

/* USER CODE BEGIN Header_taskGetNewBin */
/**
 * @brief Function implementing the getNewBin thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskGetNewBin */
__weak void taskGetNewBin(void const *argument) {
    /* USER CODE BEGIN taskGetNewBin */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskGetNewBin */
}

/* USER CODE BEGIN Header_taskManageIWDG */
/**
 * @brief Function implementing the manageIWDG thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskManageIWDG */
__weak void taskManageIWDG(void const *argument) {
    /* USER CODE BEGIN taskManageIWDG */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskManageIWDG */
}

/* USER CODE BEGIN Header_taskCreateWebPckg */
/**
 * @brief Function implementing the createWebPckg thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskCreateWebPckg */
__weak void taskCreateWebPckg(void const *argument) {
    /* USER CODE BEGIN taskCreateWebPckg */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskCreateWebPckg */
}

/* USER CODE BEGIN Header_taskWebExchange */
/**
 * @brief Function implementing the webExchange thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskWebExchange */
__weak void taskWebExchange(void const *argument) {
    /* USER CODE BEGIN taskWebExchange */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskWebExchange */
}

/* USER CODE BEGIN Header_taskGetTrainData */
/**
 * @brief Function implementing the getTrainData thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskGetTrainData */
__weak void taskGetTrainData(void const *argument) {
    /* USER CODE BEGIN taskGetTrainData */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskGetTrainData */
}

/* USER CODE BEGIN Header_taskSendTelemetry */
/**
 * @brief Function implementing the sendTelemetry thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_taskSendTelemetry */
__weak void taskSendTelemetry(void const *argument) {
    /* USER CODE BEGIN taskSendTelemetry */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
    /* USER CODE END taskSendTelemetry */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
