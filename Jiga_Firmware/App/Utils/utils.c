/*
 * utils.c
 *
 *  Created on: Jan 20, 2025
 *      Author: luisv
 */

#include "stm32l4xx_hal.h"
#include "utils.h"

void UTILS_CpuSleep(){
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

float UTILS_Map(float value, float fromMin, float fromMax, float toMin, float toMax){
    float percentage = (value - fromMin)/(fromMax - fromMin);
    float result = percentage*(toMax - toMin) + toMin;
    return result;
}

void UTILS_MovingAverageInit(movingAverage_t *self, uint8_t size){
    self->index = 0;
    self->size = size;
    for(uint8_t i = 0; i < self->size; i++){
        self->buffer[i] = 0;
    }
}

void UTILS_MovingAverageAddValue(movingAverage_t *self, uint16_t value){
    self->buffer[self->index++] = value;
    if(self->index >= self->size){
        self->index = 0;
    }
}

uint16_t UTILS_MovingAverageGetValue(movingAverage_t *self){
    uint32_t sum = 0;
    for(uint8_t i = 0; i < self->size; i++){
        sum += self->buffer[i];
    }
    return sum/self->size;
}

void UTILS_MovingAverageClear(movingAverage_t *self){
    self->index = 0;
    for(uint8_t i = 0; i < self->size; i++){
        self->buffer[i] = 0;
    }
}
