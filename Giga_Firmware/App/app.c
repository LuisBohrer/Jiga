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
#include "RingBuffer/ringBuffer.h"
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
volatile reading_t reading = READ_VOLTAGE;
volatile uint8_t newReads = 0;

uint16_t adcVoltageReads[NUMBER_OF_CHANNELS] = {0};
uint16_t adcCurrentReads[NUMBER_OF_CHANNELS] = {0};
// ADC constants and buffers //

// Uart declarations and buffers // [Section]
UART_HandleTypeDef *DISPLAY_UART = &hlpuart1;
uint8_t displayLastChar;
ringBuffer_t displayRb;
string displayLastMessage;

UART_HandleTypeDef *DEBUG_UART = &huart1;
uint8_t debugLastChar;
ringBuffer_t debugRb;

UART_HandleTypeDef *MODBUS_UART = &huart3;
uint8_t modbusLastChar;
ringBuffer_t modbusRb;
// Uart declarations and buffers //

// Timer counters and periods // [Section]
volatile uint32_t updateReadsCounter_ms = 0;
const uint32_t UPDATE_READS_PERIOD_MS = 1;
// Timer counters and periods //

// Static function declarations // [Section]
static void APP_InitUarts(void);
static void APP_InitTimers(void);
static void APP_StartAdcReadDma(reading_t typeOfRead);
static void APP_UpdateReads(void);
static void APP_TreatDisplayMessage(void);
// Static function declarations //

// Application functions // [Section]
uint8_t appStarted = 0;
void APP_init(){
    STRING_Init(&displayLastMessage);
    NEXTION_Begin(DISPLAY_UART);
    APP_StartAdcReadDma(READ_VOLTAGE);
    APP_InitUarts();
    APP_InitTimers();

    appStarted = 1;
}

void APP_poll(){
    if(!appStarted)
        return;

    APP_UpdateReads();
    APP_TreatDisplayMessage();

    UTILS_CpuSleep();
}

static void APP_InitUarts(){
    HAL_StatusTypeDef status;
    do{
        status = HAL_UART_Receive_IT(DISPLAY_UART, &displayLastChar, 1);
    } while(status != HAL_OK);
    RB_Init(&displayRb);

    do{
        status = HAL_UART_Receive_IT(DEBUG_UART, &debugLastChar, 1);
    } while(status != HAL_OK);
    RB_Init(&debugRb);

    do{
        status = HAL_UART_Receive_IT(MODBUS_UART, &modbusLastChar, 1);
    } while(status != HAL_OK);
    RB_Init(&modbusRb);
}

static void APP_InitTimers(){
    HAL_StatusTypeDef status;
    do{
        status = HAL_TIM_Base_Start_IT(&htim6);
    } while(status != HAL_OK);
}

static void APP_StartAdcReadDma(reading_t typeOfRead){
    HAL_StatusTypeDef status;
    uint16_t *readsBuffer = typeOfRead == READ_VOLTAGE ? adcVoltageReads : adcCurrentReads;
    do{
        status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) readsBuffer, NUMBER_OF_CHANNELS);
    } while(status != HAL_OK);
    reading = typeOfRead;
}

static void APP_UpdateReads(){
    if(!appStarted)
        return;

    if(updateReadsCounter_ms < UPDATE_READS_PERIOD_MS)
        return;

    if(!newReads && hadc1.State != HAL_ADC_STATE_REG_BUSY){
        APP_StartAdcReadDma(!reading);
    }

    if(newReads){
        newReads = 0;
        switch(reading){
            case READ_VOLTAGE:
                float voltageReads_V[10];
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    voltageReads_V[i] = UTILS_Map(adcVoltageReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                    NEXTION_SetComponentFloatValue(&voltageTxtBx[i], voltageReads_V[i], 2);
                }
                break;

            case READ_CURRENT:
                float currentReads_mA[10];
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    currentReads_mA[i] = UTILS_Map(adcCurrentReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_CURRENT_READ, MAX_CURRENT_READ);
                    NEXTION_SetComponentFloatValue(&currentTxtBx[i], currentReads_mA[i], 0);
                }
                break;
        }

        updateReadsCounter_ms = 0;
    }
}

static void APP_TreatDisplayMessage(){
    while(!RB_IsEmpty(&displayRb)){
        STRING_AddChar(&displayLastMessage, RB_GetByte(&displayRb));
    }

    displayResponses_t aux = NEXTION_TreatMessage(&displayLastMessage);

    if(aux != NO_MESSAGE){
        STRING_Clear(&displayLastMessage);
    }
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
        RB_PutByte(&displayRb, displayLastChar);
        do{
            status = HAL_UART_Receive_IT(DISPLAY_UART, &displayLastChar, 1);
        } while(status != HAL_OK);
    }

    else if(huart == DEBUG_UART){
        RB_PutByte(&debugRb, debugLastChar);
        do{
            status = HAL_UART_Receive_IT(DEBUG_UART, &debugLastChar, 1);
        } while(status != HAL_OK);
    }

    else if(huart == MODBUS_UART){
        RB_PutByte(&modbusRb, modbusLastChar);
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
