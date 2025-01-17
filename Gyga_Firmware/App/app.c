/*
 * app.c
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#include "app.h"
#include "String/string.h"
#include "Nextion/nextion.h"
#include "Nextion/nextionComponents.h"
#include "adc.h"

const float MIN_ADC_READ = 0;
const float MAX_ADC_READ = 4095;
const float MIN_VOLTAGE_READ = 0;
const float MAX_VOLTAGE_READ = 13.3; // V
const float MIN_CURRENT_READ = 0;
const float MAX_CURRENT_READ = 3025; // mA

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
    HAL_StatusTypeDef adcStatus;
    do{
        adcStatus = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcCurrentReads, NUMBER_OF_CHANNELS);
    } while(adcStatus != HAL_OK);
}

void APP_poll(){
    if(newReads){
        newReads = 0;
        HAL_StatusTypeDef adcStatus;
        switch(reading){
            case READ_VOLTAGE:
                float voltageReads_V[10];
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    voltageReads_V[i] = APP_map(adcVoltageReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                    NEXTION_setComponentFloatValue(&voltageTxtBx[i], voltageReads_V[i], 2);
                }
                do{
                    adcStatus = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcCurrentReads, NUMBER_OF_CHANNELS);
                } while(adcStatus != HAL_OK);
                reading = READ_CURRENT;
                break;
            case READ_CURRENT:
                float currentReads_mA[10];
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    currentReads_mA[i] = APP_map(adcCurrentReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_CURRENT_READ, MAX_CURRENT_READ);
                    NEXTION_setComponentFloatValue(&currentTxtBx[i], currentReads_mA[i], 0);
                }
                do{
                    adcStatus = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adcVoltageReads, NUMBER_OF_CHANNELS);
                } while(adcStatus != HAL_OK);
                reading = READ_VOLTAGE;
                break;
        }
    }
}

float APP_map(float value, float fromMin, float fromMax, float toMin, float toMax){
    float percentage = (value - fromMin)/(fromMax - fromMin);
    float result = percentage*(toMax - toMin) + toMin;
    return result;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
    newReads = 1;
}
