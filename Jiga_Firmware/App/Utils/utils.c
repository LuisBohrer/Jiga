/*
 * utils.c
 *
 *  Created on: Jan 20, 2025
 *      Author: luisv
 */

#include "stm32l4xx_hal.h"

void UTILS_CpuSleep(){
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

float UTILS_Map(float value, float fromMin, float fromMax, float toMin, float toMax){
    float percentage = (value - fromMin)/(fromMax - fromMin);
    float result = percentage*(toMax - toMin) + toMin;
    return result;
}
