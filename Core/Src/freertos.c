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
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

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

/* USER CODE END Variables */
osThreadId getGPSHandle;
osThreadId httpHandle;
osThreadId keepAliveHandle;
osThreadId getNewBinHandle;
osThreadId manageIWDGHandle;
osMutexId mutexGPSBufHandle;
osMutexId mutexWebHandle;
osMutexId mutexRTCHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void taskGetGPS(void const * argument);
void taskHttp(void const * argument);
void taskAlive(void const * argument);
void taskGetNewBin(void const * argument);
void taskManageIWDG(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
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

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of getGPS */
  osThreadDef(getGPS, taskGetGPS, osPriorityNormal, 0, 256);
  getGPSHandle = osThreadCreate(osThread(getGPS), NULL);

  /* definition and creation of http */
  osThreadDef(http, taskHttp, osPriorityNormal, 0, 256);
  httpHandle = osThreadCreate(osThread(http), NULL);

  /* definition and creation of keepAlive */
  osThreadDef(keepAlive, taskAlive, osPriorityNormal, 0, 256);
  keepAliveHandle = osThreadCreate(osThread(keepAlive), NULL);

  /* definition and creation of getNewBin */
  osThreadDef(getNewBin, taskGetNewBin, osPriorityNormal, 0, 256);
  getNewBinHandle = osThreadCreate(osThread(getNewBin), NULL);

  /* definition and creation of manageIWDG */
  osThreadDef(manageIWDG, taskManageIWDG, osPriorityNormal, 0, 128);
  manageIWDGHandle = osThreadCreate(osThread(manageIWDG), NULL);

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
__weak void taskGetGPS(void const * argument)
{
  /* USER CODE BEGIN taskGetGPS */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END taskGetGPS */
}

/* USER CODE BEGIN Header_taskHttp */
/**
* @brief Function implementing the http thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskHttp */
__weak void taskHttp(void const * argument)
{
  /* USER CODE BEGIN taskHttp */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END taskHttp */
}

/* USER CODE BEGIN Header_taskAlive */
/**
* @brief Function implementing the alive thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskAlive */
__weak void taskAlive(void const * argument)
{
  /* USER CODE BEGIN taskAlive */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END taskAlive */
}

/* USER CODE BEGIN Header_taskGetNewBin */
/**
* @brief Function implementing the getNewBin thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskGetNewBin */
__weak void taskGetNewBin(void const * argument)
{
  /* USER CODE BEGIN taskGetNewBin */
  /* Infinite loop */
  for(;;)
  {
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
__weak void taskManageIWDG(void const * argument)
{
  /* USER CODE BEGIN taskManageIWDG */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END taskManageIWDG */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
