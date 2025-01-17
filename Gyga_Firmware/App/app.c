/*
 * app.c
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#include "app.h"
#include "String/string.h"
#include "Nextion/nextion.h"
#include "adc.h"

typedef enum{
    READ_VOLTAGE = 0,
    READ_CURRENT
} reading_t;
reading_t reading = READ_VOLTAGE;
uint8_t newReads = 0;
uint16_t adcVoltageReads[NUMBER_OF_CHANNELS] = {0};
uint16_t adcCurrentReads[NUMBER_OF_CHANNELS] = {0};

void APP_init(){
    NEXTION_init();
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcCurrentReads, NUMBER_OF_CHANNELS);
}

void APP_poll(){
    if(newReads){
        switch(reading){
            case READ_VOLTAGE:
                NEXTION_updateReads(reading, adcVoltageReads);
                HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcCurrentReads, NUMBER_OF_CHANNELS);
                reading = READ_CURRENT;
                break;
            case READ_CURRENT:
                NEXTION_updateReads(reading, adcCurrentReads);
                HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcVoltageReads, NUMBER_OF_CHANNELS);
                reading = READ_VOLTAGE;
                break;
        }
        newReads = 0;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
//    switch(reading){
//        case READ_VOLTAGE:
//            HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcCurrentReads, NUMBER_OF_CHANNELS);
//            reading = READ_CURRENT;
//            break;
//        case READ_CURRENT:
//            HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcVoltageReads, NUMBER_OF_CHANNELS);
//            reading = READ_VOLTAGE;
//            break;
//    }
    newReads = 1;
}
