/*
 * ringBuffer.c
 *
 *  Created on: Jan 20, 2025
 *      Author: luisv
 */

#include "RingBuffer/ringBuffer.h"

void RB_init(ringBuffer_t *ringBuffer){
    ringBuffer->first = 0;
    ringBuffer->last = 0;
    ringBuffer->numberOfBytes = 0;
}

void RB_putByte(ringBuffer_t *ringBuffer, uint8_t byte){
    if(RB_isFull(ringBuffer))
        return;

    ringBuffer->buffer[ringBuffer->last++] = byte;
    ringBuffer->numberOfBytes++;
    if(ringBuffer->last >= RING_BUFFER_DEFAULT_SIZE){
        ringBuffer->last = 0;
    }
}

uint8_t RB_getByte(ringBuffer_t *ringBuffer){
    if(RB_isEmpty(ringBuffer))
        return 0;

    uint8_t byte;
    byte = ringBuffer->buffer[ringBuffer->first++];
    ringBuffer->numberOfBytes--;
    if(ringBuffer->first >= RING_BUFFER_DEFAULT_SIZE){
        ringBuffer->first = 0;
    }
    return byte;
}

uint8_t RB_isEmpty(ringBuffer_t *ringBuffer){
    return ringBuffer->numberOfBytes <= 0;
}

uint8_t RB_isFull(ringBuffer_t *ringBuffer){
    return ringBuffer->numberOfBytes >= RING_BUFFER_DEFAULT_SIZE;
}

uint16_t RB_getNumberOfBytes(ringBuffer_t *ringBuffer){
    return ringBuffer->numberOfBytes;
}
