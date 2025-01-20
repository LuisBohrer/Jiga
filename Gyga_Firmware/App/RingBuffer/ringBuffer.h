/*
 * ringBuffer.h
 *
 *  Created on: Jan 20, 2025
 *      Author: luisv
 */

#ifndef RINGBUFFER_RINGBUFFER_H_
#define RINGBUFFER_RINGBUFFER_H_

#include <stdint.h>

#define RING_BUFFER_DEFAULT_SIZE 1000

typedef struct ringBuffer_s{
        uint8_t buffer[RING_BUFFER_DEFAULT_SIZE];
        uint16_t first;
        uint16_t last;
        uint16_t numberOfBytes;
} ringBuffer_t;

void RB_init(ringBuffer_t *ringBuffer);
void RB_putByte(ringBuffer_t *ringBuffer, uint8_t byte);
uint8_t RB_getByte(ringBuffer_t *ringBuffer);
uint8_t RB_isEmpty(ringBuffer_t *ringBuffer);
uint8_t RB_isFull(ringBuffer_t *ringBuffer);
uint16_t RB_getNumberOfBytes(ringBuffer_t *ringBuffer);

#endif /* RINGBUFFER_RINGBUFFER_H_ */
