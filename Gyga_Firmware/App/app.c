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
#include "Utils/utils.h"
#include "adc.h"
#include "usart.h"
#include "tim.h"

// ADC constants and buffers // [Section]
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
// ADC constants and buffers //

// Uart declarations and buffers // [Section]
UART_HandleTypeDef *DISPLAY_UART = &hlpuart1;
uint8_t displayLastChar;

UART_HandleTypeDef *DEBUG_UART = &huart1;
uint8_t debugLastChar;

UART_HandleTypeDef *MODBUS_UART = &huart3;
uint8_t modbusLastChar;
// Uart declarations and buffers //

// Timer counters and periods // [Section]
volatile uint32_t updateReadsCounter_ms = 0;
const uint32_t UPDATE_READS_PERIOD_MS = 1;
// Timer counters and periods //

// Static function declarations // [Section]
static void APP_initUarts(void);
static void APP_initTimers(void);
static void APP_startAdcReadDma(uint16_t *readsBuffer, reading_t typeOfRead);
static void APP_updateReads(void);
// Static function declarations //

// Application functions // [Section]
uint8_t appStarted = 0;
void APP_init(){
    NEXTION_init();
    APP_startAdcReadDma(adcVoltageReads, READ_VOLTAGE);
    APP_initUarts();
    APP_initTimers();

    appStarted = 1;
}

void APP_poll(){
    if(!appStarted)
        return;

    APP_updateReads();
}

static void APP_initUarts(){
    HAL_StatusTypeDef status;
    do{
        status = HAL_UART_Receive_IT(DISPLAY_UART, &displayLastChar, 1);
    } while(status != HAL_OK);
    do{
        status = HAL_UART_Receive_IT(DEBUG_UART, &debugLastChar, 1);
    } while(status != HAL_OK);
    do{
        status = HAL_UART_Receive_IT(MODBUS_UART, &modbusLastChar, 1);
    } while(status != HAL_OK);
}

static void APP_initTimers(){
    HAL_StatusTypeDef status;
    do{
        status = HAL_TIM_Base_Start_IT(&htim6);
    } while(status != HAL_OK);
}

static void APP_startAdcReadDma(uint16_t *readsBuffer, reading_t typeOfRead){
    HAL_StatusTypeDef status;
    do{
        status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) readsBuffer, NUMBER_OF_CHANNELS);
    } while(status != HAL_OK);
    reading = typeOfRead;
}

static void APP_updateReads(){
    if(!appStarted)
        return;

    if(updateReadsCounter_ms < UPDATE_READS_PERIOD_MS)
        return;

    if(newReads){
        newReads = 0;
        switch(reading){
            case READ_VOLTAGE:
                float voltageReads_V[10];
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    voltageReads_V[i] = UTILS_map(adcVoltageReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                    NEXTION_setComponentFloatValue(&voltageTxtBx[i], voltageReads_V[i], 2);
                }
                APP_startAdcReadDma(adcCurrentReads, READ_CURRENT);
                break;

            case READ_CURRENT:
                float currentReads_mA[10];
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    currentReads_mA[i] = UTILS_map(adcCurrentReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_CURRENT_READ, MAX_CURRENT_READ);
                    NEXTION_setComponentFloatValue(&currentTxtBx[i], currentReads_mA[i], 0);
                }
                APP_startAdcReadDma(adcVoltageReads, READ_VOLTAGE);
                break;
        }

        updateReadsCounter_ms = 0;
    }

    UTILS_cpuSleep();
}
// Application functions //

// Callbacks // [Section]
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
    newReads = 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if(!appStarted)
        return;

    HAL_StatusTypeDef status;
    if(huart == DISPLAY_UART){
        // trata mensagem displayHAL_StatusTypeDef status;

        do{
            status = HAL_UART_Receive_IT(DISPLAY_UART, &displayLastChar, 1);
        } while(status != HAL_OK);
    }

    else if(huart == DEBUG_UART){
        // trata mensagem coletora

        do{
            status = HAL_UART_Receive_IT(DEBUG_UART, &debugLastChar, 1);
        } while(status != HAL_OK);
    }

    else if(huart == MODBUS_UART){
        // trata mensagem modbus

        do{
            status = HAL_UART_Receive_IT(MODBUS_UART, &modbusLastChar, 1);
        } while(status != HAL_OK);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(!appStarted)
        return;

    // 1ms
    if(htim == &htim6){
        if(updateReadsCounter_ms < UPDATE_READS_PERIOD_MS)
            updateReadsCounter_ms++;
    }
}
// Callbacks //
