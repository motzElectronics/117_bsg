/*
 * circularBuffer.c
 *
 *  Created on: 10 ���. 2019 �.
 *      Author: annsi
 */

#include "../Utils/Inc/circularBuffer.h"

// u8 circularBuffer[SZ_CIRCULAR_BUF] = {0};

// CircularBuffer dmaBuffer;

void cBufInit(CircularBuffer* cbuf, u8* buf, u16 szBuf, CircTypeBuf type) {
    cbuf->buf = buf;
    cbuf->max = szBuf;
    cbuf->type = type;
    cBufReset(cbuf);
    D(printf("cBufInit()\r\n"));
}

void cBufReset(CircularBuffer* cbuf) {
    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->writeAvailable = cbuf->max;
    cbuf->readAvailable = 0;
    cbuf->numPckgInBuf = 0;
    cbuf->curLenMsg = 0;
    memset(cbuf->buf, '\0', cbuf->max);
}

// u8 cBufIsEmpty(CBufHandle cbuf){
//	return (!cbuf->isFull && (cbuf->head == cbuf->tail));
//}

// u8 cBufGetQuantity(CBufHandle cbuf){
//
//	u8 sz = cbuf->max;
//	if(!cbuf->isFull){
//		sz = cbuf->head >= cbuf->tail ? cbuf->head - cbuf->tail :
//cbuf->max + cbuf->head - cbuf->tail;
//	}
//
//	return sz;
//}

void cBufWriteToBuf(CBufHandle cbuf, u8* data, u8 sz) {
    if (cbuf->writeAvailable > sz) {
        cbuf->remainWrite = cbuf->max - cbuf->head;
        if (sz > cbuf->remainWrite) {
            memcpy(cbuf->buf + cbuf->head, data, cbuf->remainWrite);
            memcpy(cbuf->buf, data + cbuf->remainWrite, sz - cbuf->remainWrite);
        } else {
            memcpy(cbuf->buf + cbuf->head, data, sz);
        }

        cbuf->numPckgInBuf++;
        cbuf->head = (cbuf->head + sz) % cbuf->max;
        cbuf->writeAvailable -= sz;
        cbuf->readAvailable += sz;
    } else {
        switch (cbuf->type) {
            case CIRC_TYPE_PCKG_GNSS_RMC:
                D(printf("%s: CIRC_TYPE_PCKG_GNSS_RMC\r\n", CIRC_MSG_FULL));
                break;
            case CIRC_TYPE_GNSS:
                D(printf("%s: CIRC_TYPE_GNSS\r\n", CIRC_MSG_FULL));
                break;
        }
    }
}

u16 cBufRead(CBufHandle cbuf, u8* dist, CircTypeBuf typeBuf, u8 sz) {
    u16 lenMsg;
    switch (typeBuf) {
        case CIRC_TYPE_SIM_UART:
            if ((lenMsg = getLenMsgSimUart(cbuf))) {
                copyGetDatafromBuf(cbuf, dist, lenMsg, CIRC_TYPE_SIM_UART);
            }
            break;
        case CIRC_TYPE_GNSS:
            if ((lenMsg = getLenMsgUblox(cbuf)))
                copyGetDatafromBuf(cbuf, dist, lenMsg, CIRC_TYPE_GNSS);
            break;
    }
    return lenMsg;
}

void copyGetDatafromBuf(CBufHandle cbuf, u8* dist, u16 sz, CircTypeBuf type) {
    cbuf->remainRead = cbuf->max - cbuf->tail;
    if (sz > cbuf->remainRead) {
        memcpy(dist, cbuf->buf + cbuf->tail, cbuf->remainRead);
        memcpy(dist + cbuf->remainRead, cbuf->buf, sz - cbuf->remainRead);
        memset(cbuf->buf + cbuf->tail, '\0', cbuf->remainRead);
        memset(cbuf->buf, '\0', sz - cbuf->remainRead);

    } else {
        memcpy(dist, cbuf->buf + cbuf->tail, sz);
        memset(cbuf->buf + cbuf->tail, '\0', sz);
    }
    if (type == CIRC_TYPE_SIM_UART) {
        cbuf->tail = (cbuf->tail + sz + CIRC_LEN_ENDS) % cbuf->max;
        cbuf->writeAvailable += CIRC_LEN_ENDS;
        cbuf->readAvailable -= CIRC_LEN_ENDS;
    } else if (type == CIRC_TYPE_GNSS) {
        cbuf->tail = cbuf->head;
    }
    cbuf->writeAvailable += sz;
    cbuf->readAvailable -= sz;
    --(cbuf->numPckgInBuf);
}

u8 getLenMsgSimUart(CBufHandle cbuf) {
    u16 lenMsg = 0;
    u16 tail = cbuf->tail;
    if (cbuf->numPckgInBuf > 0) {
        while (tail != cbuf->head) {
            if (cbuf->buf[tail] == CIRC_END1_MSG &&
                cbuf->buf[(tail + 1) % cbuf->max] == CIRC_END2_MSG) {
                cbuf->buf[tail] = '\0';
                cbuf->buf[(tail + 1) % cbuf->max] = '\0';
                break;
            }
            tail = (tail + 1) % cbuf->max;
            lenMsg++;
        }
    }
    return lenMsg;
}

u16 getLenMsgUblox(CircularBuffer* cbuf) {
    u16 lenMsg = 1;
    u16 tail = cbuf->tail;
    u16 head;
    while (cbuf->buf[tail] != '\0') {
        if (cbuf->buf[tail] == CIRC_HEAD_SIGN_UBLOX) {
            head = (tail + 1) % cbuf->max;
            while (cbuf->buf[head] != '\0') {
                if (cbuf->buf[head] == CIRC_HEAD_SIGN_UBLOX) {
                    cbuf->tail = tail;
                    cbuf->head = head;
                    return lenMsg;
                }
                lenMsg++;
                head = (head + 1) % cbuf->max;
            }
            return 0;
        }
        tail = (tail + 1) % cbuf->max;
    }
    return 0;
}
