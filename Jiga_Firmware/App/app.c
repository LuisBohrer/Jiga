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
#include "Modbus/modbus.h"
#include "adc.h"
#include "usart.h"
#include "tim.h"
#include "rtc.h"
#include <stdio.h>
#include <math.h>

// ADC constants and buffers // [Section]
#define NUMBER_OF_CHANNELS 10

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

// Display constants // [Section]
const uint16_t NUMBER_OF_DIGITS_IN_BOX = 4;
// Display constants //

// Uart declarations and buffers // [Section]
typedef enum{
    BAUD_RATE_9600 = 0,
    BAUD_RATE_19200,
    BAUD_RATE_115200,
} uartBaudRate_t;

typedef enum {
    STOP_BITS_0_5 = 0,
    STOP_BITS_1,
    STOP_BITS_1_5,
    STOP_BITS_2,
} uartStopBits_t;

typedef enum {
    PARITY_NONE = 0,
    PARITY_EVEN,
    PARITY_ODD,
} uartParity_t;

UART_HandleTypeDef *DISPLAY_UART = &hlpuart1;
uint8_t displayLastChar;
ringBuffer_t displayRb;
string displayLastMessage;

UART_HandleTypeDef *DEBUG_UART = &huart1;
uint8_t debugLastChar;
ringBuffer_t debugRb;
string debugLastMessage;

UART_HandleTypeDef *MODBUS_UART = &huart3;
uint8_t modbusLastChar;
ringBuffer_t modbusRb;
string modbusLastMessage;
// Uart declarations and buffers //

// Modbus declarations // [Section]
modbusHandler_t modbusHandler;
const uint8_t DEVICE_ADDRESS = 0; // 0 representa o mestre
const uint16_t MODBUS_MAX_TIME_BETWEEN_BYTES_MS = 500;
uint8_t modbusEnabled;
// Modbus declarations //

// Timer counters and periods // [Section]
volatile uint32_t updateReadsCounter_ms = 0;
const uint32_t UPDATE_READS_PERIOD_MS = 1;

volatile uint32_t modbusTimeBetweenByteCounter_ms = 0;
// Timer counters and periods //

// Static function declarations // [Section]
static void APP_InitUarts(void);
static void APP_InitTimers(void);
static void APP_StartAdcReadDma(reading_t typeOfRead);
static void APP_UpdateReads(void);
static void APP_TreatDisplayMessage(void);
static void APP_SendLog(void);
static void APP_SendPeriodicReads(void);
static void APP_TreatDebugMessage(void);
static void APP_TreatModbusMessage(void);
static void APP_EnableModbus(void);
static void APP_DisableModbus(void);
static void APP_DisableUartInterrupt(UART_HandleTypeDef *huart);
static uint8_t APP_EnableUartInterrupt(UART_HandleTypeDef *huart);
static void APP_UpdateUartConfigs(UART_HandleTypeDef *huart, uint8_t *uartBuffer, uartBaudRate_t baudRate, uartStopBits_t stopBits, uartParity_t parity);
static void APP_SendReadsMinute(void);
static void APP_SetRtcTime(RTC_HandleTypeDef *hrtc, uint8_t seconds, uint8_t minutes, uint8_t hours);
static void APP_SetRtcDate(RTC_HandleTypeDef *hrtc, uint8_t day, uint8_t month, uint8_t year);
static void APP_AddRtcTimestampToString(string *String, RTC_HandleTypeDef *baseTime);
// Static function declarations //

// Application functions // [Section]
uint8_t appStarted = 0;
void APP_init(){
    APP_InitUarts();
    APP_InitTimers();

    NEXTION_Begin(DISPLAY_UART);
    COMM_Begin(DEBUG_UART);
    MODBUS_Begin(&modbusHandler, E_RS485_GPIO_Port, E_RS485_Pin, MODBUS_UART, DEVICE_ADDRESS);

    APP_EnableModbus();

    APP_StartAdcReadDma(READ_VOLTAGE);

    APP_SetRtcTime(&hrtc, 55, 38, 14);
    APP_SetRtcDate(&hrtc, 07, 02, 25);

    appStarted = 1;
}

void APP_poll(){
    if(!appStarted)
        return;

    APP_UpdateReads();

    APP_TreatDisplayMessage();
    APP_TreatDebugMessage();
    APP_TreatModbusMessage();

    APP_SendReadsMinute();

    UTILS_CpuSleep();
}
// Application functions //

// Init functions // [Section]
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

    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16); // 1 seg
}

static void APP_StartAdcReadDma(reading_t typeOfRead){
    HAL_StatusTypeDef status;
    uint16_t *readsBuffer = typeOfRead == READ_VOLTAGE ? adcVoltageReads : adcCurrentReads;
    do{
        status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) readsBuffer, NUMBER_OF_CHANNELS);
    } while(status != HAL_OK);
    reading = typeOfRead;
}
// Init functions //

// Adc reading functions // [Section]
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
        uint16_t integerSpaces = NUMBER_OF_DIGITS_IN_BOX;
        uint16_t decimalSpaces = 0;
        switch(reading){
            case READ_VOLTAGE:
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    convertedVoltageReads_V[i] = UTILS_Map(adcVoltageReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                    for(uint32_t order = pow(10, NUMBER_OF_DIGITS_IN_BOX - 1); order >= 1; order/=10){
                        if(convertedVoltageReads_V[i] > order){
                            break;
                        }
                        integerSpaces--;
                        if(decimalSpaces < 2){
                            decimalSpaces++;
                        }
                    }
                    NEXTION_SetComponentFloatValue(&voltageTxtBx[i], convertedVoltageReads_V[i], integerSpaces, decimalSpaces);
                }
                break;

            case READ_CURRENT:
                for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
                    convertedCurrentReads_mA[i] = UTILS_Map(adcCurrentReads[i],
                            MIN_ADC_READ, MAX_ADC_READ,
                            MIN_CURRENT_READ, MAX_CURRENT_READ);
                    for(uint32_t order = pow(10, NUMBER_OF_DIGITS_IN_BOX - 1); order >= 1; order/=10){
                        if(convertedCurrentReads_mA[i] > order){
                            break;
                        }
                        integerSpaces--;
                        if(decimalSpaces < 2){
                            decimalSpaces++;
                        }
                    }
                    NEXTION_SetComponentFloatValue(&currentTxtBx[i], convertedCurrentReads_mA[i], integerSpaces, decimalSpaces);
                }
                break;
        }
        updateReadsCounter_ms = 0;
    }
}
// Adc reading functions //

// Message treatment functions // [Section]
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
                APP_UpdateUartConfigs(MODBUS_UART,
                        &modbusLastChar,
                        STRING_GetChar(&debugLastMessage, 3),
                        STRING_GetChar(&debugLastMessage, 4),
                        STRING_GetChar(&debugLastMessage, 5));
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

static void APP_TreatModbusMessage(){
    if(!modbusEnabled)
        return;
    if(modbusTimeBetweenByteCounter_ms >= MODBUS_MAX_TIME_BETWEEN_BYTES_MS &&
            STRING_GetLength(&modbusLastMessage) > 0){
        STRING_Clear(&modbusLastMessage);
        return;
    }

    while(!RB_IsEmpty(&modbusRb)){
        STRING_AddChar(&modbusLastMessage, RB_GetByte(&modbusRb));
    }

    if(MODBUS_VerifyCrc(STRING_GetBuffer(&modbusLastMessage), STRING_GetLength(&modbusLastMessage))
            == MODBUS_NO_ERROR){
        modbusOpcodes_t opcode = STRING_GetChar(&modbusLastMessage, 1);
        switch(opcode){
            case READ_COILS:
                break;
            default:
                break;
        }
        STRING_Clear(&modbusLastMessage);
    }
}
// Message treatment functions //

// Uart handling functions // [Section]
static void APP_DisableUartInterrupt(UART_HandleTypeDef *huart){
    HAL_UART_Abort_IT(huart);
}

static uint8_t APP_EnableUartInterrupt(UART_HandleTypeDef *huart){
    HAL_StatusTypeDef status;
    if(huart == DISPLAY_UART){
        status = HAL_UART_Receive_IT(huart, &displayLastChar, 1);
    }
    else if(huart == DEBUG_UART){
        status = HAL_UART_Receive_IT(huart, &debugLastChar, 1);
    }
    else if(huart == MODBUS_UART){
        status = HAL_UART_Receive_IT(huart, &modbusLastChar, 1);
    }
    return (status == HAL_OK) || (status == HAL_BUSY);
}

static void APP_UpdateUartConfigs(UART_HandleTypeDef *huart, uint8_t *uartBuffer, uartBaudRate_t baudRate, uartStopBits_t stopBits, uartParity_t parity){
    HAL_UART_Abort_IT(huart);
    HAL_UART_DeInit(huart); // se começar a dar problema, copiar da coletora

    switch(baudRate){
        case BAUD_RATE_9600:
            huart->Init.BaudRate = 9600;
            break;
        case BAUD_RATE_19200:
            huart->Init.BaudRate = 19200;
            break;
        case BAUD_RATE_115200:
            huart->Init.BaudRate = 115200;
            break;
    }

    switch(stopBits){
        case STOP_BITS_0_5:
            huart->Init.StopBits = UART_STOPBITS_0_5;
            break;
        case STOP_BITS_1:
            huart->Init.StopBits = UART_STOPBITS_1;
            break;
        case STOP_BITS_1_5:
            huart->Init.StopBits = UART_STOPBITS_1_5;
            break;
        case STOP_BITS_2:
            huart->Init.StopBits = UART_STOPBITS_2;
            break;
    }

    switch(parity){
        case PARITY_NONE:
            huart->Init.Parity = UART_PARITY_NONE;
            huart->Init.WordLength = UART_WORDLENGTH_8B;
            break;
        case PARITY_EVEN:
            huart->Init.Parity = UART_PARITY_EVEN;
            huart->Init.WordLength = UART_WORDLENGTH_9B; // O wordlength considera o bit de paridade
            break;
        case PARITY_ODD:
            huart->Init.Parity = UART_PARITY_ODD;
            huart->Init.WordLength = UART_WORDLENGTH_9B; // O wordlength considera o bit de paridade
            break;
    }

    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_UART_Receive_IT(huart, uartBuffer, 1);
}
// Uart handling functions //

// Specific utility functions // [Section]
static void APP_SendLog(){
    string logMessage;
    STRING_Init(&logMessage);
    APP_AddRtcTimestampToString(&logMessage, &hrtc);
    STRING_AddCharString(&logMessage, "\n\r");
    COMM_SendStartPacket();
    COMM_SendAck(ACK_LOGS);
    COMM_SendString(&logMessage);

    for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
        STRING_AddCharString(&logMessage, "\n\r");
        uint8_t line[100] = {'\0'};
        uint8_t length = sprintf((char*)line, "[LOG] Read %d: Voltage = %.2f V ; Current = %.2f mA\n\r", i, convertedVoltageReads_V[i], convertedCurrentReads_mA[i]);
        COMM_SendChar(line, length);
    }
    COMM_SendEndPacket();
}

static void APP_SendPeriodicReads(){
    string message;
    STRING_Init(&message);
    STRING_AddCharString(&message, "\n\r");
    APP_AddRtcTimestampToString(&message, &hrtc);
    STRING_AddCharString(&message, "\n\r");
    COMM_SendString(&message);

    for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
        STRING_Init(&message);
        char line[100] = {'\0'};
        sprintf(line, "Read %d: Voltage = %.2f V ; Current = %.2f mA", i, convertedVoltageReads_V[i], convertedCurrentReads_mA[i]);
        STRING_AddCharString(&message, line);
        STRING_AddCharString(&message, "\n\r");
        COMM_SendString(&message);
    }
}

static void APP_EnableModbus(){
    if(modbusEnabled)
        return;

    HAL_GPIO_WritePin(LIGA_RS485_GPIO_Port, LIGA_RS485_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LIGA_RS485__GPIO_Port, LIGA_RS485__Pin, GPIO_PIN_RESET);

    while(APP_EnableUartInterrupt(MODBUS_UART) == 0);

    RB_ClearBuffer(&modbusRb);

    modbusEnabled = 1;
}

static void APP_DisableModbus(){
    if(!modbusEnabled)
        return;

    APP_DisableUartInterrupt(MODBUS_UART);

    HAL_GPIO_WritePin(LIGA_RS485_GPIO_Port, LIGA_RS485_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGA_RS485__GPIO_Port, LIGA_RS485__Pin, GPIO_PIN_SET);

    modbusEnabled = 0;
}

RTC_TimeTypeDef currentTime;
static void APP_SendReadsMinute(){
    static uint8_t lastMessageMinute = 61;
    HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, NULL, RTC_FORMAT_BIN); // necessario pra nao travar o rtc

    if(lastMessageMinute != currentTime.Minutes){
        APP_SendPeriodicReads();
        lastMessageMinute = currentTime.Minutes;
    }
}

static void APP_SetRtcTime(RTC_HandleTypeDef *hrtc, uint8_t seconds, uint8_t minutes, uint8_t hours){
    RTC_TimeTypeDef newTime = {0};
    newTime.Seconds = seconds;
    newTime.Minutes = minutes;
    newTime.Hours = hours;

    HAL_StatusTypeDef status;
    do{
        status = HAL_RTC_SetTime(hrtc, &newTime, RTC_FORMAT_BIN);
    } while(status != HAL_OK);
}

static void APP_SetRtcDate(RTC_HandleTypeDef *hrtc, uint8_t day, uint8_t month, uint8_t year){
    RTC_DateTypeDef newDate = {0};
    newDate.Date = day;
    newDate.Month = month;
    newDate.Year = year;

    HAL_StatusTypeDef status;
    do{
        status = HAL_RTC_SetDate(hrtc, &newDate, RTC_FORMAT_BIN);
    } while(status != HAL_OK);
}

static void APP_AddRtcTimestampToString(string *String, RTC_HandleTypeDef *baseTime){
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    char buffer[100];

    HAL_RTC_GetDate(baseTime, &date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(baseTime, &time, RTC_FORMAT_BIN);

    sprintf(buffer, "%.2d/%.2d/%.4d %.2d:%.2d:%.2d",
            date.Date,
            date.Month,
            date.Year + 2000,
            time.Hours,
            time.Minutes,
            time.Seconds);
    STRING_AddCharString(String, buffer);
}
// Specific utility functions //

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
        modbusTimeBetweenByteCounter_ms = 0;
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
        if(modbusTimeBetweenByteCounter_ms < MODBUS_MAX_TIME_BETWEEN_BYTES_MS)
            modbusTimeBetweenByteCounter_ms++;
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc){

}
// Callbacks //
