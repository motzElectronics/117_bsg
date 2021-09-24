/*
 * sim800.c
 *
 *  Created on: Feb 20, 2020
 *      Author: bls
 */

#include "../xDrvrs/Inc/simcom.h"

// //#define PEER_CLOSED 1 //
// (u8*)"http://188.242.176.25:8080/api/add/measures"; #define PEER_CLOSED 1 //
// (u8*)"http://ptsv2.com/t/rt98e-1582717549/post";

static char simBufCmd[COMMAND_BUF_SZ];
static char simBufError[COMMAND_BUF_SZ];

u8 isJson = 1;

void simInit() {
    char* retMsg;
    char* token;
    u8    fail = 0;
    u8    simBadAnsw = 0;
    u8    isInit = 0;

    bsg.isTCPOpen = 0;

    while (!isInit) {
        bsg.stat.simResetCnt++;
        simHardwareReset();

        gnssInit();

        simTxATCmd(SIM_CMD_ATE0, SIM_SZ_CMD_ATE0, 1000);
        retMsg = simTxATCmd(SIM_CMD_AT, SIM_SZ_CMD_AT, 1000);
        token = strtok(retMsg, SIM_SEPARATOR_TEXT);
        if (token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
        D(printf("simInit AT: %s\r\n", token));
        if ((strcmp(token, SIM_OK_TEXT)) != 0) {
            D(printf("ERROR: simInit() AT: %s\r\n", token));
            simBadAnsw = (simBadAnsw + 1) % 16;
            if (!simBadAnsw) {
                D(printf("WARINTING!: T O T A L  R E S E T\r\n"));
                osDelay(3000);
                HAL_NVIC_SystemReset();
            }
        } else {
            // bsg.erFlags.simAT = 0;
            // simCmd("CGNSCMD=0,\"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0*35\"", NULL, 3, SIM_OK_TEXT);
            osDelay(5000);
            if (SIM_GPS_INIT() != SIM_SUCCESS) {
                fail++;
                // bsg.erFlags.simSAPBR = 1;
                if (fail > 20) {
                    fail = 0;
                    HAL_NVIC_SystemReset();
                }
                D(printf("ERROR: NOT CONNECT GPS\r\n"));
            } else {
                isInit = 1;
                // bsg.erFlags.simSAPBR = 0;
            }
        }
    }

    D(printf("OK: simInit()\r\n"));
}

char* simGetStatusAnsw(u32 timeout) {
    waitIdle("", &(uInfoSim.irqFlags), 200, timeout);
    if (uInfoSim.irqFlags.isIrqIdle) {
        uInfoSim.irqFlags.isIrqIdle = 0;
        return (char*)uInfoSim.pRxBuf;
    } else {
        return SIM_NO_RESPONSE_TEXT;
    }
}

char* simTxATCmd(char* command, u16 sz, u32 timeout) {
    uInfoSim.irqFlags.isIrqIdle = 0;
    uartTx(command, sz, &uInfoSim);
    return simGetStatusAnsw(timeout);
}

u8 simCmd(char* cmdCode, char* params, u8 retriesCnt, char* SUCCESS_RET) {
    char* retMsg;
    char* token;
    memset(simBufCmd, '\0', COMMAND_BUF_SZ);
    if (params == NULL)
        sprintf((char*)simBufCmd, "AT+%s\r\n", cmdCode);
    else {
        sprintf((char*)simBufCmd, "AT+%s=%s\r\n", cmdCode, params);
    }
    for (; retriesCnt > 0; --retriesCnt) {
        retMsg = simTxATCmd(simBufCmd, strlen(simBufCmd), 20000);
        token = strtok(retMsg, SIM_SEPARATOR_TEXT);
        if (token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
        if (SUCCESS_RET != NULL &&
            strcmp((const char*)token, (const char*)SUCCESS_RET)) {
            copyStr(simBufError, token, COMMAND_BUF_SZ);
            sprintf((char*)simBufError + strlen(token), "\r\n");
            if (strcmp((const char*)simBufError, (const char*)simBufCmd) == 0) {
                return SIM_RESTART;
            }
            D(printf("ERROR: %s ret: %s\r\n", simBufCmd, token));
        } else {
            // D(printf("OK: %s ret: %s\r\n", simBufCmd, token));
            return SIM_SUCCESS;
        }
    }
    return SIM_FAIL;
}

void copyStr(char* dist, char* source, u16 distSz) {
    memset(dist, '\0', distSz);
    sprintf(dist, "%s", source);
}

char* simDownloadData(char* data, u16 sz) {
    return simTxATCmd(data, sz, 90000);
}

u8 simCheckCSQ() {
    u8    csq = 0;
    char* retMsg;
    char* token;

    retMsg = simTxATCmd("AT+CSQ\r\n", 8, 1000);  // check signal level
    token = strtok(retMsg, SIM_SEPARATOR_TEXT);
    csq = (token != NULL) && (strlen(token) > 8) ? atoi(token + 6) : 0;
    bsg.csq = csq;
    return csq;
}

void simOn() {
    HAL_GPIO_WritePin(SIM_PWR_EN_GPIO_Port, SIM_PWR_EN_Pin, GPIO_PIN_SET);
    osDelay(1500);
    HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, GPIO_PIN_RESET);
    osDelay(1000);
    HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, GPIO_PIN_SET);
    osDelay(1500);
    HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, GPIO_PIN_RESET);
    osDelay(5000);
}

void simOff() {
    HAL_GPIO_WritePin(SIM_PWR_EN_GPIO_Port, SIM_PWR_EN_Pin, GPIO_PIN_RESET);
    osDelay(3000);
}

void simHardwareReset() {
    D(printf("WARINIG!: R E S E T !\r\n"));
    simOff();
    simOn();
}

void simReset() { simInit(); }

u8 simTCPCheckStatus(const char* stat, u16 timeout, u16 delay) {
    char* token;
    while (timeout > delay) {
        timeout -= delay;
        if (simCmd(SIM_CIPSTATUS, NULL, 3, SIM_OK_TEXT) != SIM_SUCCESS) {
            return SIM_FAIL;
        }
        osDelay(delay);
        token = strtok((char*)uInfoSim.pRxBuf + 6, SIM_SEPARATOR_TEXT);
        if (token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
        if (strcmp((const char*)token, stat) != 0) {
            D(printf("ER: %s instead %s\r\n", token, stat));
        } else {
            D(printf("%s\r\n", token));
            return SIM_SUCCESS;
        }
    }
    return SIM_FAIL;
}

u8 simTCPinit() {
    if (simCmd(SIM_CIPSHUT, NULL, 3, SIM_OK_CIPSHUT) != SIM_SUCCESS) {
        return SIM_FAIL;
    }

    if (simCmd(SIM_CGATT, "1", 3, SIM_OK_TEXT) != SIM_SUCCESS) {
        return SIM_FAIL;
    }

    if (simTCPCheckStatus(SIM_CIPSTAT_INIT, 200, 50) != SIM_SUCCESS) {
        return SIM_FAIL;
    }

    if (simCmd(SIM_CSTT, "\"internet\",\"gdata\",\"gdata\"", 3, SIM_OK_TEXT) ==
        SIM_FAIL) {  // maybe can delete gdata. test it.
        return SIM_FAIL;
    }
    if (simTCPCheckStatus(SIM_CIPSTAT_START, 200, 50) != SIM_SUCCESS) {
        return SIM_FAIL;
    }

    if (simCmd(SIM_CIICR, NULL, 3, SIM_OK_TEXT) == SIM_FAIL) {
        return SIM_FAIL;
    }
    if (simTCPCheckStatus(SIM_CIPSTAT_GPRSACT, 200, 50) != SIM_SUCCESS) {
        return SIM_FAIL;
    }

    if (simCmd(SIM_CIFSR, NULL, 3, NULL) == SIM_FAIL) {
        return SIM_FAIL;
    }
    if (simTCPCheckStatus(SIM_CIPSTAT_STATUS, 200, 50) != SIM_SUCCESS) {
        return SIM_FAIL;
    }
    return SIM_SUCCESS;
}

u8 simTCPOpen() {
    static char params[40];
    char*       token;
    memset(params, '\0', 40);
    sprintf(params, "\"%s\",\"%s\",%d", (char*)"TCP", urls.tcpAddr,
            urls.tcpPort);
    if (simCmd(SIM_CIPSTART, params, 3, SIM_OK_TEXT) == SIM_FAIL) {
        return SIM_FAIL;
    }
    osDelay(1500);
    token = strtok((char*)uInfoSim.pRxBuf + 6, SIM_SEPARATOR_TEXT);
    if (token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
    if (strcmp((const char*)token, (const char*)"CONNECT OK") != 0) {
        return SIM_FAIL;
    }
    if (simTCPCheckStatus(SIM_CIPSTAT_CON_OK, 7000, 200) != SIM_SUCCESS) {
        return SIM_FAIL;
    }
    return SIM_SUCCESS;
}

u8 simTCPSend(u8* data, u16 sz) {
    static char params[8];
    char*       token;
    char*       retMsg;

    u32 ttt = HAL_GetTick();

    if (sz == 0) {
        D(printf("ERROR SZ\r\n"));
        return SIM_FAIL;
    }
    // D(printf("simDownloadData() sz:%d\r\n", sz));
    memset(params, '\0', 8);
    sprintf(params, "%d", sz);
    if (simCmd(SIM_CIPSEND, params, 1, "> ") == SIM_FAIL) {
        return SIM_FAIL;
    }
    retMsg = simDownloadData(data, sz);
    token = strtok(retMsg, SIM_SEPARATOR_TEXT);
    if (token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;

    ttt = HAL_GetTick() - ttt;

    if (strcmp((const char*)token, (const char*)"SEND OK") == 0) {
        D(printf("OK: simTCPSend() time %d\r\n", ttt));
        return SIM_SUCCESS;
    } else if (strcmp((const char*)token, (const char*)"SEND FAIL") == 0) {
        D(printf("ER: simDownloadData() %s time %d\r\n", token, ttt));
        return SIM_FAIL;
    } else {
        D(printf("ER: simDownloadData() %s time %d\r\n", token, ttt));
        return SIM_TIMEOUT;
    }
    return SIM_SUCCESS;
}

long long simGetPhoneNum() {
    char* retMsg;
    retMsg = simTxATCmd(SIM_CMD_CNUM, SIM_SZ_CMD_CNUM, 2000);
    if (retMsg[0] != '\0') {
        printf("Num is %s\r\n", retMsg);
        return atoll(retMsg + 15);
    } else
        return 0;
}

u8 procReturnStatus(u8 ret) {
    static u8 notSend = 0;
    if (ret != TCP_OK) {
        notSend++;
    } else {
        notSend = 0;
    }

    if (ret == TCP_SEND_ER) {
        D(printf("TCP_SEND_ER %d!\r\n\r\n", notSend));
        //if (notSend == 5) {
        //D(printf("UNABLE TO SEND 5!\r\n"));
        simReset();
        ret = TCP_SEND_ER_LOST_PCKG;
        notSend = 0;
        //}
    } else if (ret == TCP_CONNECT_ER) {
        D(printf("CONNECT ERROR - TOTAL RESET!\r\n"));
        osDelay(1000);
        NVIC_SystemReset();
    } else if (ret != TCP_OK) {
        D(printf("UNABLE TO SEND!\r\n"));
        simReset();
        ret = TCP_SEND_ER_LOST_PCKG;
        notSend = 0;
    }

    return ret;
}

u8 openTcp() {
    u8 ret = TCP_OK;
    if (!waitGoodCsq(5400)) {
        D(printf("ER: waitGoodCsq\r\n"));
        ret = TCP_CSQ_ER;
        bsg.stat.simBadCsqCnt++;
    }
    if (ret == TCP_OK && simTCPinit() != SIM_SUCCESS) {
        D(printf("ER: simTCPinit\r\n"));
        ret = TCP_INIT_ER;
    }
    if (ret == TCP_OK && simTCPOpen() != SIM_SUCCESS) {
        D(printf("ER: simTCPOpen\r\n"));
        ret = TCP_OPEN_ER;
    }
    if (ret == TCP_OK) {
        bsg.isTCPOpen = 1;
        bsg.stat.simOpenCnt++;
    }
    return procReturnStatus(ret);
}

u8 sendTcp(u8* data, u16 sz) {
    u8 ret = TCP_OK;
    u8 cnt = 0;
    while (!bsg.isTCPOpen) {
        if (cnt == 15) {
            ret = TCP_CONNECT_ER;
            break;
        }
        cnt++;
        ret = openTcp();
    }
    if (ret == TCP_OK && !waitGoodCsq(90)) {
        D(printf("ER: waitGoodCsq\r\n"));
        ret = TCP_CSQ_ER;
    }
    if (ret == TCP_OK && simTCPSend(data, sz) != SIM_SUCCESS) {
        D(printf("ER: simTCPSend\r\n"));
        ret = TCP_SEND_ER;
    }
    if (ret == TCP_OK) {
        bsg.stat.simSendCnt++;
    } else {
        bsg.stat.simErrCnt++;
    }
    if (bsg.csq < 15) {
        bsg.stat.simLowCsqCnt++;
    } else if (bsg.csq < 20) {
        bsg.stat.simLowCsqCnt++;
    } else {
        bsg.stat.simHighCsqCnt++;
    }
    return procReturnStatus(ret);
}

void gnssInit() {
    D(printf("gnssInit()\r\n"));
    HAL_GPIO_WritePin(GNSS_EN_GPIO_Port, GNSS_EN_Pin, GPIO_PIN_SET);
    osDelay(3000);
    HAL_UART_Transmit(uInfoGnss.pHuart, (u8*)"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0*35\r\n",
                      sizeof("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0*35\r\n"), 1000);

    uartRxDma(&uInfoGnss);
}