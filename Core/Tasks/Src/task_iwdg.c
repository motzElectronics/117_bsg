#include "../Tasks/Inc/task_iwdg.h"

#include "../xDrvrs/Inc/spiflash.h"

extern IWDG_HandleTypeDef hiwdg;

extern u8 isRxNewFirmware;

u16 iwdgTaskReg;
u32 iwdgErrCount;

void taskManageIWDG(void const* argument) {
    iwdgTaskReg = 0;
    iwdgErrCount = 0;

    for (;;) {
        if ((isRxNewFirmware && (iwdgTaskReg & IWDG_TASK_REG_NEW_BIN) == IWDG_TASK_REG_NEW_BIN) ||
            (!isRxNewFirmware && (iwdgTaskReg & IWDG_TASK_REG_ALL) == IWDG_TASK_REG_ALL)) {
            iwdgTaskReg = 0;
            iwdgErrCount = 0;
        } else {
            iwdgErrCount++;
        }

        if (iwdgErrCount > 400) {
            spiFlashSaveInfo();
            NVIC_SystemReset();
        }

        HAL_IWDG_Refresh(&hiwdg);
        osDelay(3000);
    }
}