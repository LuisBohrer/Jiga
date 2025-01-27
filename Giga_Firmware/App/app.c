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
#include "Comm/comm.h"
#include "adc.h"
#include "usart.h"
#include "tim.h"
#include <stdio.h>

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
float convertedVoltageReads_V[NUMBER_OF_CHANNELS] = {0};

uint16_t adcCurrentReads[NUMBER_OF_CHANNELS] = {0};
float convertedCurrentReads_mA[NUMBER_OF_CHANNELS] = {0};
// ADC constants and buffers //

// Uart declarations and buffers // [Section]
UART_HandleTypeDef *DISPLAY_UART = &hlpuart1;
//UART_HandleTypeDef *DISPLAY_UART = &huart1;
uint8_t displayLastChar;
ringBuffer_t displayRb;
string displayLastMessage;

UART_HandleTypeDef *DEBUG_UART = &huart1;
//UART_HandleTypeDef *DEBUG_UART = &hlpuart1;
uint8_t debugLastChar;
ringBuffer_t debugRb;
string debugLastMessage;

UART_HandleTypeDef *MODBUS_UART = &huart3;
uint8_t modbusLastChar;
ringBuffer_t modbusRb;
string modbusLastMessage;
// Uart declarations and buffers //

// Timer counters and periods // [Section]
volatile uint32_t updateReadsCounter_ms = 0;
const uint32_t UPDATE_READS_PERIOD_MS = 1;

volatile uint32_t sendLogCounter_ms = 0;
const uint32_t SEND_LOG_PERIOD_MS = 1000;
// Timer counters and periods //

// Static function declarations // [Section]
static void APP_InitUarts(void);
static void APP_InitTimers(void);
static void APP_StartAdcReadDma(reading_t typeOfRead);
static void APP_UpdateReads(void);
static void APP_TreatDisplayMessage(void);
static void APP_SendLog(void);
static void APP_TreatDebugMessage(void);
// Static function declarations //

// Application functions // [Section]
uint8_t appStarted = 0;
void APP_init(){

    NEXTION_Begin(DISPLAY_UART);
    COMM_Begin(DEBUG_UART);

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
    APP_TreatDebugMessage();

    UTILS_CpuSleep();
}

static void APP_InitUarts(){
    HAL_StatusTypeDef status;
    do{
        status = HAL_UART_Receive_IT(DISPLAY_UART, &displayLastChar, 1);
    } while(status != HAL_OK);
    RB_Init(&displayRb);
    STRING_Init(&displayLastMessage);

    do{
        status = HAL_UART_Receive_IT(DEBUG_UART, &debugLastChar, 1);
    } while(status != HAL_OK);
    RB_Init(&debugRb);
    STRING_Init(&debugLastMessage);

    do{
        status = HAL_UART_Receive_IT(MODBUS_UART, &modbusLastChar, 1);
    } while(status != HAL_OK);
    RB_Init(&modbusRb);
    STRING_Init(&modbusLastMessage);
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
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    convertedVoltageReads_V[i] = UTILS_Map(adcVoltageReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                    NEXTION_SetComponentFloatValue(&voltageTxtBx[i], convertedVoltageReads_V[i], 2);
                }
                break;

            case READ_CURRENT:
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    convertedCurrentReads_mA[i] = UTILS_Map(adcCurrentReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_CURRENT_READ, MAX_CURRENT_READ);
                    NEXTION_SetComponentFloatValue(&currentTxtBx[i], convertedCurrentReads_mA[i], 0);
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

    if(STRING_GetLength(&displayLastMessage) > 0){
        displayResponses_t aux = NEXTION_TreatMessage(&displayLastMessage);

        if(aux != NO_MESSAGE){
            STRING_Clear(&displayLastMessage);
        }
    }
}

static void APP_TreatDebugMessage(){
    while(!RB_IsEmpty(&debugRb)){
        STRING_AddChar(&debugLastMessage, RB_GetByte(&debugRb));
    }

    if(STRING_GetLength(&debugLastMessage) > 0){
        debugRequest_t request = COMM_TreatResponse(&debugLastMessage);
        if(request == INCOMPLETE_REQUEST)
            return;

//        if(request == LOGS){
//            APP_SendLog();
//            STRING_Clear(&debugLastMessage);
//            return;
//        }

        COMM_SendStartPacket();
        switch(request){
            case INVALID_REQUEST:
                COMM_SendAck(NACK);
                break;

            case SEND_VOLTAGE_READS:
                COMM_SendAck(ACK_VOLTAGE_READS);
                COMM_SendValues16Bits(adcVoltageReads, NUMBER_OF_CHANNELS);
                break;

            case SEND_CURRENT_READS:
                COMM_SendAck(ACK_CURRENT_READS);
                COMM_SendValues16Bits(adcCurrentReads, NUMBER_OF_CHANNELS);
                break;

            case SEND_ALL_READS:
                COMM_SendAck(ACK_ALL_READS);
                COMM_SendValues16Bits(adcVoltageReads, NUMBER_OF_CHANNELS);
                COMM_SendValues16Bits(adcCurrentReads, NUMBER_OF_CHANNELS);
                break;

            case SET_MODBUS_CONFIG:
                COMM_SendAck(ACK_MODBUS_CONFIG);
                break;

            case CHANGE_SCALE:
                COMM_SendAck(ACK_CHANGE_SCALE);
                break;

            case LOGS:
                COMM_SendAck(ACK_LOGS);
                APP_SendLog();
                break;

            default:
                COMM_SendAck(NACK);
                break;
        }
        COMM_SendEndPacket();
    }
    STRING_Clear(&debugLastMessage);
}

static void APP_SendLog(){
    string logMessage;

    for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
        STRING_Init(&logMessage);
        STRING_AddCharString(&logMessage, "\n\r");
        char line[100] = {'\0'};
        sprintf(line, "[LOG] Read %d: Voltage = %.2f V ; Current = %.2f mA", i, convertedVoltageReads_V[i], convertedCurrentReads_mA[i]);
        STRING_AddCharString(&logMessage, line);
        STRING_AddCharString(&logMessage, "\n\r");
        COMM_SendStartPacket();
        COMM_SendAck(ACK_LOGS);
        COMM_SendString(&logMessage);
        COMM_SendEndPacket();
    }

    sendLogCounter_ms = 0;
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
        if(sendLogCounter_ms < SEND_LOG_PERIOD_MS)
            sendLogCounter_ms++;
    }
}
// Callbacks //
