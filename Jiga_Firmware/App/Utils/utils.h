/*
 * utils.h
 *
 *  Created on: Jan 20, 2025
 *      Author: luisv
 */

#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#define MOVING_AVERAGE_MAX_BUFFER_SIZE 20

typedef struct movingAverage_s{
        uint16_t buffer[MOVING_AVERAGE_MAX_BUFFER_SIZE];
        uint8_t index;
        uint8_t size;
} movingAverage_t;

void UTILS_CpuSleep(void);
float UTILS_Map(float value, float fromMin, float fromMax, float toMin, float toMax);
void UTILS_MovingAverageInit(movingAverage_t *self, uint8_t size);
void UTILS_MovingAverageAddValue(movingAverage_t *self, uint16_t value);
uint16_t UTILS_MovingAverageGetValue(movingAverage_t *self);
void UTILS_MovingAverageClear(movingAverage_t *self);

#endif /* UTILS_UTILS_H_ */
