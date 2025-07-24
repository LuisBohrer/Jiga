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
#include "Leds/leds.h"
#include "Modbus/registers.h"
#include "Eeprom/eeprom.h"
#include "Eeprom/memoryMap.h"
#include "i2c.h"

// ADC constants and buffers // [Section]
#define NUMBER_OF_CHANNELS 10

uint16_t adcVoltageCalibrationMin[NUMBER_OF_CHANNELS] = {[0 ... NUMBER_OF_CHANNELS-1] = 0};
uint16_t adcVoltageCalibrationMax[NUMBER_OF_CHANNELS] = {[0 ... NUMBER_OF_CHANNELS-1] = 4095};
uint16_t adcCurrentCalibrationMin[NUMBER_OF_CHANNELS] = {28, 14, 19, 11, 14, 20, 18, 13, 20, 16};
uint16_t adcCurrentCalibrationMax[NUMBER_OF_CHANNELS] = {4176, 4087, 4155, 4028, 4148, 4073, 4172, 4022, 4132, 4110};

const uint16_t MIN_ADC_READ = 0;
const uint16_t MAX_ADC_READ = 4095;

const float MIN_VOLTAGE_READ = 0;
const float MAX_VOLTAGE_READ = 13.3; // V
const float MIN_CURRENT_READ = 0;
//const float MAX_CURRENT_READ = 3025; // mA
const float MAX_CURRENT_READ = 2944; // mA

typedef enum{
    READ_VOLTAGE = 0,
    READ_CURRENT
} reading_t;
volatile reading_t reading = READ_VOLTAGE;
volatile uint8_t newReads = 0;

#define MODBUS_NUMBER_OF_DEVICES 2 // conta com o mestre
uint16_t adcVoltageReads[NUMBER_OF_CHANNELS] = {0};
uint16_t convertedVoltageReads_int[NUMBER_OF_CHANNELS] = {0};
float convertedVoltageReads_V[MODBUS_NUMBER_OF_DEVICES][NUMBER_OF_CHANNELS] = {0};

uint16_t adcCurrentReads[NUMBER_OF_CHANNELS] = {0};
uint16_t convertedCurrentReads_int[NUMBER_OF_CHANNELS] = {0};
float convertedCurrentReads_mA[MODBUS_NUMBER_OF_DEVICES][NUMBER_OF_CHANNELS] = {0};

const uint16_t* inputRegistersMap[NUMBER_OF_CHANNELS*2] = {0};

const uint8_t MOVING_AVERAGE_BUFFER_SIZE = 10;
movingAverage_t currentMovingAverage[NUMBER_OF_CHANNELS];
movingAverage_t voltageMovingAverage[NUMBER_OF_CHANNELS];
// ADC constants and buffers //

// Display constants // [Section]
const uint16_t NUMBER_OF_DIGITS_IN_BOX = 4;
const string nextionStartBytes = {{'#', '#'}, 2};
typedef enum displayOpcodes_e{
    SET_AS_MASTER = 0,
} displayOpcodes_t;
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
uint8_t DEVICE_ADDRESS = 0; // 0 representa o mestre

uint8_t modbusMaster = 0; // flag para identificar o mestre
uint8_t modbusWaitingForResponse = 0;

uint8_t modbusEnabled;
// Modbus declarations //

// Timer counters and periods // [Section]
volatile uint32_t updateDisplayCounter_ms = 0;
const uint32_t UPDATE_DISPLAY_PERIOD_MS = 50;

volatile uint32_t updateReadsCounter_ms = 0;
const uint32_t UPDATE_READS_PERIOD_MS = 5;

volatile uint32_t modbusTimeBetweenByteCounter_ms = 0;
volatile uint32_t debugTimeBetweenByteCounter_ms = 0;
const uint16_t MODBUS_MAX_TIME_BETWEEN_BYTES_MS = 500; // tempo maximo ate reconhecer como fim da mensagem
const uint16_t MASTER_BUFFER_PERIOD = 1500; // tempo a mais esperado pelo mestre para receber uma resposta
const uint16_t DEBUG_MAX_TIME_BETWEEN_BYTES_MS = 500;

volatile uint32_t requestReadsCounter_ms = 0;
const uint32_t REQUEST_READS_PERIOD_MS = 50; // tempo minimo de espera entre requisicoes

volatile uint32_t testDisplayConnectionCounter_ms = 0;
const uint32_t TEST_DISPLAY_CONNECTION_PERIOD_MS = 2000;
// Timer counters and periods //

// Board supply enables declarations // [Section]
typedef enum{
    SUPPLY_5V = 1,
    SUPPLY_DISPLAY = 1 << 1,
    SUPPLY_RS485 = 1 << 2,
    SUPPLY_ALL = SUPPLY_5V | SUPPLY_DISPLAY | SUPPLY_RS485,
} supplyEnable_t;
// Board supply enables declarations //

// Static function declarations // [Section]
static void APP_InitUarts(void);
static void APP_InitTimers(void);
static void APP_EnableSupplies(uint8_t supplyFlags);
void APP_DisableSupplies(uint8_t supplyFlags);
static void APP_StartAdcReadDma(reading_t typeOfRead);
static void APP_InitModbus(void);
static void APP_UpdateReads(void);
static void APP_RequestReads(void);
static void APP_UpdateDisplay(void);
static void APP_TreatDisplayMessage(void);
static void APP_SendLog(void);
static void APP_TreatDebugMessage(void);
static void APP_TreatModbusMessage(void);
static void APP_TreatMasterRequest(string *request);
static void APP_TreatSlaveResponse(string *request);
static void APP_EnableModbus(void);
void APP_DisableModbus(void);
static void APP_DisableUartInterrupt(UART_HandleTypeDef *huart);
static uint8_t APP_EnableUartInterrupt(UART_HandleTypeDef *huart);
static void APP_UpdateUartConfigs(UART_HandleTypeDef *huart, uint8_t *uartBuffer, uartBaudRate_t baudRate, uartStopBits_t stopBits, uartParity_t parity);
static void APP_SetRtcTime(RTC_HandleTypeDef *hrtc, uint8_t seconds, uint8_t minutes, uint8_t hours);
static void APP_SetRtcDate(RTC_HandleTypeDef *hrtc, uint8_t day, uint8_t month, uint8_t year);
static void APP_AddRtcTimestampToString(string *String, RTC_HandleTypeDef *baseTime);
// Static function declarations //

// Application functions // [Section]
uint8_t appStarted = 0;
void APP_init(){
    APP_InitUarts();
    APP_InitTimers();

    APP_EnableSupplies(SUPPLY_ALL);

    NEXTION_Begin(DISPLAY_UART);
    COMM_Begin(DEBUG_UART);
    APP_InitModbus();

    APP_EnableModbus();

    for(uint8_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
        UTILS_MovingAverageInit(&voltageMovingAverage[channel], MOVING_AVERAGE_BUFFER_SIZE);
        UTILS_MovingAverageInit(&currentMovingAverage[channel], MOVING_AVERAGE_BUFFER_SIZE);

    }
//    EEPROM_Read(&hi2c1, (uint8_t*)adcVoltageCalibrationMin, VOLTAGE_CALIBRATION_MIN_0, sizeof(adcVoltageCalibrationMin)/sizeof(uint8_t));
//    EEPROM_Read(&hi2c1, (uint8_t*)adcVoltageCalibrationMax, VOLTAGE_CALIBRATION_MAX_0, sizeof(adcVoltageCalibrationMax)/sizeof(uint8_t));
//    EEPROM_Read(&hi2c1, (uint8_t*)adcCurrentCalibrationMin, CURRENT_CALIBRATION_MIN_0, sizeof(adcCurrentCalibrationMin)/sizeof(uint8_t));
//    EEPROM_Read(&hi2c1, (uint8_t*)adcCurrentCalibrationMax, CURRENT_CALIBRATION_MAX_0, sizeof(adcCurrentCalibrationMax)/sizeof(uint8_t));
    APP_StartAdcReadDma(READ_VOLTAGE);

    APP_SetRtcTime(&hrtc, 55, 38, 14);
    APP_SetRtcDate(&hrtc, 07, 02, 25);

    vLEDS_SetLedState(1, GPIO_PIN_SET);
    vLEDS_SetLedState(2, GPIO_PIN_RESET);
    appStarted = 1;
    //APP_CalibrateSensorsMin();
}

void APP_poll(){
    if(!appStarted)
        return;

    APP_UpdateReads();

    APP_TreatDisplayMessage();
    APP_TreatDebugMessage();
    APP_TreatModbusMessage();

    if(modbusMaster){
        APP_UpdateDisplay();
        APP_RequestReads();
    }
    //APP_SendReadsMinute();

    //UTILS_CpuSleep(); // nao liga o display
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

static void APP_InitModbus(void){
    DEVICE_ADDRESS |= (!HAL_GPIO_ReadPin(ADDR1_GPIO_Port, ADDR1_Pin));
    DEVICE_ADDRESS |= (!HAL_GPIO_ReadPin(ADDR2_GPIO_Port, ADDR2_Pin) << 1);
    DEVICE_ADDRESS |= (!HAL_GPIO_ReadPin(ADDR3_GPIO_Port, ADDR3_Pin) << 2);
    DEVICE_ADDRESS |= (!HAL_GPIO_ReadPin(ADDR4_GPIO_Port, ADDR4_Pin) << 3);
    DEVICE_ADDRESS += 0x6C;

    // Organizando o array de acordo com a lista de registradores em regs.h
    for(uint8_t i = 0; i < NUMBER_OF_CHANNELS; i++){
        inputRegistersMap[i*2] = &convertedVoltageReads_int[i];
        inputRegistersMap[i*2 + 1] = &convertedCurrentReads_int[i];
    }

    MODBUS_Begin(&modbusHandler, E_RS485_GPIO_Port, E_RS485_Pin, MODBUS_UART, DEVICE_ADDRESS);
}

static void APP_EnableSupplies(uint8_t supplyFlags){
    if(supplyFlags & SUPPLY_5V){
        HAL_GPIO_WritePin(LIGA5V_GPIO_Port, LIGA5V_Pin, GPIO_PIN_SET);
    }
    if(supplyFlags & SUPPLY_DISPLAY){
        HAL_GPIO_WritePin(DIS_EN_GPIO_Port, DIS_EN_Pin, GPIO_PIN_SET);
    }
    if(supplyFlags & SUPPLY_RS485){
        HAL_GPIO_WritePin(LIGA_RS485_GPIO_Port, LIGA_RS485_Pin, GPIO_PIN_SET);
    }
}

void APP_DisableSupplies(uint8_t supplyFlags){
    if(supplyFlags & SUPPLY_5V){
        HAL_GPIO_WritePin(LIGA5V_GPIO_Port, LIGA5V_Pin, GPIO_PIN_RESET);
    }
    if(supplyFlags & SUPPLY_DISPLAY){
        HAL_GPIO_WritePin(DIS_EN_GPIO_Port, DIS_EN_Pin, GPIO_PIN_RESET);
    }
    if(supplyFlags & SUPPLY_RS485){
        HAL_GPIO_WritePin(LIGA5V_GPIO_Port, LIGA5V_Pin, GPIO_PIN_RESET);
    }
}

static void APP_StartAdcReadDma(reading_t typeOfRead){
    HAL_StatusTypeDef status;
    HAL_GPIO_WritePin(SELECT_GPIO_Port, SELECT_Pin, typeOfRead == READ_VOLTAGE);
    HAL_Delay(5); // importante pra ter leituras certas
    uint16_t *readsBuffer = typeOfRead == READ_VOLTAGE ? adcVoltageReads : adcCurrentReads;
    do{
        status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*) readsBuffer, NUMBER_OF_CHANNELS);
    } while(status != HAL_OK);
    HAL_Delay(5); // importante pra ter leituras certas
    reading = typeOfRead;
}
// Init functions //

// Adc reading functions // [Section]
static void APP_UpdateReads(){
    if(!appStarted)
        return;

    if(!newReads && hadc1.State != HAL_ADC_STATE_REG_BUSY){
        APP_StartAdcReadDma(!reading);
    }

    if(updateReadsCounter_ms < UPDATE_READS_PERIOD_MS){
        return;
    }

    if(newReads){
        newReads = 0;
        switch(reading){
            case READ_VOLTAGE:
                for(uint16_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
                    UTILS_MovingAverageAddValue(&voltageMovingAverage[channel], adcVoltageReads[channel]);
                    uint16_t voltageValue = UTILS_MovingAverageGetValue(&voltageMovingAverage[channel]);
                    if(voltageValue < adcVoltageCalibrationMin[channel] + 5){
                        voltageValue = 0;
                    }

                    // Converte e salva no registrador do modbus
                    convertedVoltageReads_int[channel] = UTILS_Map(voltageValue,
                            adcVoltageCalibrationMin[channel], adcVoltageCalibrationMax[channel],
                            MIN_ADC_READ, MAX_ADC_READ);
                    // Converte para enviar pro display
                    convertedVoltageReads_V[0][channel] = UTILS_Map(voltageValue,
                            adcVoltageCalibrationMin[channel], adcVoltageCalibrationMax[channel],
                            MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                }
                break;

            case READ_CURRENT:
                for(uint16_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
                    UTILS_MovingAverageAddValue(&currentMovingAverage[channel], adcCurrentReads[channel]);
                    uint16_t currentValue = UTILS_MovingAverageGetValue(&currentMovingAverage[channel]);
                    if(currentValue < adcCurrentCalibrationMin[channel] + 5){
                        currentValue = 0;
                    }

                    // Converte e salva no registrador do modbus
                    convertedCurrentReads_int[channel] = UTILS_Map(currentValue,
                            adcCurrentCalibrationMin[channel], adcCurrentCalibrationMax[channel],
                            MIN_ADC_READ, MAX_ADC_READ);
                    // Converte para enviar pro display
                    convertedCurrentReads_mA[0][channel] = UTILS_Map(currentValue,
                            adcCurrentCalibrationMin[channel], adcCurrentCalibrationMax[channel],
                            MIN_CURRENT_READ, MAX_CURRENT_READ);
                }
                break;
        }
        APP_StartAdcReadDma(!reading);
    }
}

static void APP_RequestReads(void){
    if(!appStarted){
        return;
    }
    if(!modbusMaster){
        return;
    }
    if(modbusTimeBetweenByteCounter_ms >= MODBUS_MAX_TIME_BETWEEN_BYTES_MS){
        modbusWaitingForResponse = 0;
    }
    if(modbusWaitingForResponse){
        return;
    }
    STRING_Clear(&modbusLastMessage);

    static uint8_t slaveNumber = 0;
    MODBUS_ReadInputRegisters(&modbusHandler, 0x6C + slaveNumber, 0, 20);

    if(++slaveNumber >= MODBUS_NUMBER_OF_DEVICES){
        slaveNumber = 0;
    }

    modbusWaitingForResponse = 1;
    requestReadsCounter_ms = 0;
    modbusTimeBetweenByteCounter_ms = 0;
}

static void APP_UpdateDisplay(void){
    if(!appStarted){
        return;
    }
    if(updateDisplayCounter_ms < UPDATE_DISPLAY_PERIOD_MS){
        return;
    }

    for(uint8_t placa = 0; placa < MODBUS_NUMBER_OF_DEVICES; placa++){
        for(uint8_t i = 0; i < NUMBER_OF_CHANNELS; i++){
            uint16_t integerSpaces =  UTILS_GetIntegerSpacesFromFloat(convertedVoltageReads_V[placa][i]);
            uint16_t decimalSpaces = NUMBER_OF_DIGITS_IN_BOX - integerSpaces;
            if(decimalSpaces > 2){
                decimalSpaces = 2;
            }

            if(placa == 0){
                NEXTION_SetComponentFloatValue(&voltageTxtBx1[i], convertedVoltageReads_V[placa][i], integerSpaces, decimalSpaces);
            }
            else{
                NEXTION_SetComponentFloatValue(&voltageTxtBx2[i], convertedVoltageReads_V[placa][i], integerSpaces, decimalSpaces);
            }

            integerSpaces = UTILS_GetIntegerSpacesFromFloat(convertedCurrentReads_mA[placa][i]);
            decimalSpaces = NUMBER_OF_DIGITS_IN_BOX - integerSpaces;
            if(decimalSpaces > 2){
                decimalSpaces = 2;
            }

            if(placa == 0){
                NEXTION_SetComponentFloatValue(&currentTxtBx1[i], convertedCurrentReads_mA[placa][i], integerSpaces, decimalSpaces);
            }
            else{
                NEXTION_SetComponentFloatValue(&currentTxtBx2[i], convertedCurrentReads_mA[placa][i], integerSpaces, decimalSpaces);
            }
            HAL_Delay(1);
        }
    }

    updateDisplayCounter_ms = 0;
}
// Adc reading functions //

// Message treatment functions // [Section]
static void APP_TreatDisplayMessage(){
    if(!appStarted){
        RB_ClearBuffer(&displayRb);
        return;
    }
    while(!RB_IsEmpty(&displayRb)){
        STRING_AddChar(&displayLastMessage, RB_GetByte(&displayRb));
    }
    RB_ClearBuffer(&displayRb);
    if(testDisplayConnectionCounter_ms >= TEST_DISPLAY_CONNECTION_PERIOD_MS){
        APP_DisableUartInterrupt(DISPLAY_UART); // reseta a uart se nao recebeu mensagens por muito tempo
        APP_EnableUartInterrupt(DISPLAY_UART);
        testDisplayConnectionCounter_ms = 0;
    }
    if(STRING_GetLength(&displayLastMessage) <= 0){
        return;
    }
    testDisplayConnectionCounter_ms = 0; // recebeu uma mensagem do display -> conexao ok

    displayResponses_t aux = NEXTION_TreatMessage(&displayLastMessage);

    if(aux == NO_MESSAGE || aux == INCOMPLETE_MESSAGE){
        return;
    }
    if(aux != VALID_MESSAGE){
        STRING_Clear(&displayLastMessage);
        return;
    }
    if(!STRING_CompareStrings(&displayLastMessage, &nextionStartBytes, STRING_GetLength(&nextionStartBytes))){
        STRING_Clear(&displayLastMessage);
        return;
    }

    displayOpcodes_t opcode = STRING_GetChar(&displayLastMessage, 2);
    if(opcode == SET_AS_MASTER){
        if(!modbusMaster){
            DEVICE_ADDRESS = 0;
            modbusMaster = 1;
            MODBUS_Begin(&modbusHandler, E_RS485_GPIO_Port, E_RS485_Pin, MODBUS_UART, DEVICE_ADDRESS);
        }
    }
    STRING_Clear(&displayLastMessage);
}

static void APP_TreatDebugMessage(){
    if(!appStarted){
        RB_ClearBuffer(&debugRb);
        return;
    }
    debugRequest_t request = INCOMPLETE_REQUEST;
    while(!RB_IsEmpty(&debugRb)){
        STRING_AddChar(&debugLastMessage, RB_GetByte(&debugRb));
        request = COMM_TreatResponse(&debugLastMessage);
    }
    if(STRING_GetLength(&debugLastMessage) <= 0){
        return;
    }

    uint8_t length = 0;
    if(debugTimeBetweenByteCounter_ms >= DEBUG_MAX_TIME_BETWEEN_BYTES_MS){
        STRING_Clear(&debugLastMessage);
    }
    if(request == INCOMPLETE_REQUEST){
        return;
    }

    uint8_t channel = 0;
    uint32_t address = 0;
    HAL_StatusTypeDef status = 0;
    COMM_SendStartPacket();
    switch(request){
        case INVALID_REQUEST:
            COMM_SendAck(INVALID_REQUEST);
            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case SEND_VOLTAGE_READS:
            length = sizeof(convertedVoltageReads_int)/sizeof(uint8_t);
            COMM_SendAck(SEND_VOLTAGE_READS);
            COMM_SendChar(&length, 1);
            COMM_SendValues16Bits(convertedVoltageReads_int, NUMBER_OF_CHANNELS);
            break;

        case SEND_CURRENT_READS:
            length = sizeof(convertedCurrentReads_int)/sizeof(uint8_t);
            COMM_SendAck(SEND_CURRENT_READS);
            COMM_SendChar(&length, 1);
            COMM_SendValues16Bits(convertedCurrentReads_int, NUMBER_OF_CHANNELS);
            break;

        case SEND_ALL_READS:
            length = sizeof(convertedVoltageReads_int)/sizeof(uint8_t)
                    + sizeof(convertedCurrentReads_int)/sizeof(uint8_t);
            COMM_SendAck(SEND_ALL_READS);
            COMM_SendChar(&length, 1);
            COMM_SendValues16Bits(convertedVoltageReads_int, NUMBER_OF_CHANNELS);
            COMM_SendValues16Bits(convertedCurrentReads_int, NUMBER_OF_CHANNELS);
            break;

        case SET_MODBUS_CONFIG:
            COMM_SendAck(SET_MODBUS_CONFIG);
            APP_UpdateUartConfigs(MODBUS_UART,
                    &modbusLastChar,
                    STRING_GetChar(&debugLastMessage, 3),
                    STRING_GetChar(&debugLastMessage, 4),
                    STRING_GetChar(&debugLastMessage, 5));

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case CALIBRATE_VOLTAGE_MIN:
            COMM_SendAck(CALIBRATE_VOLTAGE_MIN);

            channel = STRING_GetChar(&debugLastMessage, 4);
            adcVoltageCalibrationMin[channel] = UTILS_MovingAverageGetValue(&voltageMovingAverage[channel]);
            address = VOLTAGE_CALIBRATION_MIN_0 + 2*channel;
            length = 2;
            do{
                status = EEPROM_Write(&hi2c1, (uint8_t*)&adcVoltageCalibrationMin[channel], address, length);
            } while(status != HAL_OK);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case CALIBRATE_VOLTAGE_MAX:
            COMM_SendAck(CALIBRATE_VOLTAGE_MAX);

            channel = STRING_GetChar(&debugLastMessage, 4);
            adcVoltageCalibrationMax[channel] = UTILS_MovingAverageGetValue(&voltageMovingAverage[channel]);
            address = VOLTAGE_CALIBRATION_MAX_0 + 2*channel;
            length = 2;
            do{
                status = EEPROM_Write(&hi2c1, (uint8_t*)&adcVoltageCalibrationMax[channel], address, length);
            } while(status != HAL_OK);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case CALIBRATE_CURRENT_MIN:
            COMM_SendAck(CALIBRATE_CURRENT_MIN);

            channel = STRING_GetChar(&debugLastMessage, 4);
            adcCurrentCalibrationMin[channel] = UTILS_MovingAverageGetValue(&currentMovingAverage[channel]);
            address = CURRENT_CALIBRATION_MIN_0 + 2*channel;
            length = 2;
            do{
                status = EEPROM_Write(&hi2c1, (uint8_t*)&adcCurrentCalibrationMin[channel], address, length);
            } while(status != HAL_OK);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case CALIBRATE_CURRENT_MAX:
            COMM_SendAck(CALIBRATE_CURRENT_MAX);

            channel = STRING_GetChar(&debugLastMessage, 4);
            adcCurrentCalibrationMax[channel] = UTILS_MovingAverageGetValue(&currentMovingAverage[channel]);
            address = CURRENT_CALIBRATION_MAX_0 + 2*channel;
            length = 2;
            do{
                status = EEPROM_Write(&hi2c1, (uint8_t*)&adcCurrentCalibrationMax[channel], address, length);
            } while(status != HAL_OK);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case RESET_VOLTAGE_MIN:
            COMM_SendAck(RESET_VOLTAGE_MIN);
            for(uint8_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
                adcVoltageCalibrationMin[channel] = MIN_ADC_READ;
            }
            length = sizeof(adcCurrentCalibrationMax)/sizeof(uint8_t);
            EEPROM_Write(&hi2c1, (uint8_t*)adcCurrentCalibrationMax, VOLTAGE_CALIBRATION_MIN_0, length);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case RESET_VOLTAGE_MAX:
            COMM_SendAck(RESET_VOLTAGE_MAX);
            for(uint8_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
                adcVoltageCalibrationMax[channel] = MAX_ADC_READ;
            }
            length = sizeof(adcVoltageCalibrationMax)/sizeof(uint8_t);
            EEPROM_Write(&hi2c1, (uint8_t*)adcVoltageCalibrationMax, VOLTAGE_CALIBRATION_MAX_0, length);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case RESET_CURRENT_MIN:
            COMM_SendAck(RESET_CURRENT_MIN);
            for(uint8_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
                adcCurrentCalibrationMin[channel] = MIN_ADC_READ;
            }
            length = sizeof(adcCurrentCalibrationMin)/sizeof(uint8_t);
            EEPROM_Write(&hi2c1, (uint8_t*)adcCurrentCalibrationMin, CURRENT_CALIBRATION_MIN_0, length);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case RESET_CURRENT_MAX:
            COMM_SendAck(RESET_CURRENT_MAX);
            for(uint8_t channel = 0; channel < NUMBER_OF_CHANNELS; channel++){
                adcCurrentCalibrationMax[channel] = MAX_ADC_READ;
            }
            length = sizeof(adcCurrentCalibrationMax)/sizeof(uint8_t);
            EEPROM_Write(&hi2c1, (uint8_t*)adcCurrentCalibrationMax, CURRENT_CALIBRATION_MAX_0, length);

            length = 0;
            COMM_SendChar(&length, 1);
            break;

        case LOGS:
            COMM_SendAck(LOGS);
            APP_SendLog();
            break;

        default:
            COMM_SendAck(INVALID_REQUEST);
            length = 0;
            COMM_SendChar(&length, 1);
            break;
    }
    COMM_SendEndPacket();
    STRING_Clear(&debugLastMessage);
}

static void APP_TreatModbusMessage(){
    if(!appStarted){
        RB_ClearBuffer(&modbusRb);
        return;
    }
    if(!modbusEnabled){
        return;
    }
    if(modbusTimeBetweenByteCounter_ms >= MODBUS_MAX_TIME_BETWEEN_BYTES_MS){
        STRING_Clear(&modbusLastMessage);
        modbusWaitingForResponse = 0;
        vLEDS_SetLedState(2, GPIO_PIN_RESET);
        return;
    }
    if(RB_IsEmpty(&modbusRb)){
        return;
    }
    vLEDS_SetLedState(2, GPIO_PIN_SET);

    while(!RB_IsEmpty(&modbusRb)){
        STRING_AddChar(&modbusLastMessage, RB_GetByte(&modbusRb));
    }
    RB_ClearBuffer(&modbusRb);
    if(MODBUS_VerifyCrc(STRING_GetBuffer(&modbusLastMessage), STRING_GetLength(&modbusLastMessage))
            != MODBUS_NO_ERROR){
        return;
    }

    if(modbusMaster){
        APP_TreatSlaveResponse(&modbusLastMessage);
        modbusWaitingForResponse = 0;
    }
    else{
        if(STRING_GetLength(&modbusLastMessage) != 0){
            APP_TreatMasterRequest(&modbusLastMessage);
        }
    }
    STRING_Clear(&modbusLastMessage);
    vLEDS_SetLedState(2, GPIO_PIN_RESET);
}

static void APP_TreatMasterRequest(string *request){
    if(!appStarted){
        return;
    }

    modbusError_t messageError = MODBUS_VerifyCrc(STRING_GetBuffer(request), STRING_GetLength(request));
    if(messageError == MODBUS_INCORRECT_CRC){
        MODBUS_SendError(&modbusHandler, MODBUS_INCORRECT_CRC);
        return;
    }
    if(messageError == MODBUS_INCOMPLETE_MESSAGE){
        MODBUS_SendError(&modbusHandler, MODBUS_TIMEOUT);
        return;
    }

    if(messageError == MODBUS_NO_ERROR){
        MODBUS_UpdateHandler(&modbusHandler, STRING_GetBuffer(request));
        if(modbusHandler.requestId != DEVICE_ADDRESS){
            return;
        }

        uint8_t responseBuffer[100] = {0};
        uint16_t responseBufferIndex = 0;
        switch(modbusHandler.opcode){
            case READ_INPUT_REGISTERS:
                if(modbusHandler.firstRegister + modbusHandler.qttRegisters > NUMBER_OF_INPUT_REGISTERS){
                    MODBUS_SendError(&modbusHandler, MODBUS_ILLEGAL_DATA_ADDRESS);
                    break;
                }

                for(uint8_t i = 0; i < modbusHandler.qttRegisters; i++){
                    responseBuffer[responseBufferIndex++] = (*inputRegistersMap[modbusHandler.firstRegister + i]) >> 8;
                    responseBuffer[responseBufferIndex++] = (*inputRegistersMap[modbusHandler.firstRegister + i]) & 0xFF;
                }
                MODBUS_SendResponse(&modbusHandler, responseBuffer, responseBufferIndex);
                break;
            default:
                MODBUS_SendError(&modbusHandler, MODBUS_ILLEGAL_FUNCTION);
                break;
        }
    }
}

static void APP_TreatSlaveResponse(string *response){
    if(!appStarted){
        return;
    }

    modbusError_t messageError = MODBUS_VerifyWithHandler(&modbusHandler, STRING_GetBuffer(response), STRING_GetLength(response));
    if(messageError != MODBUS_NO_ERROR){
        STRING_Clear(&modbusLastMessage);
        return;
    }

    switch(modbusHandler.opcode){
        case READ_INPUT_REGISTERS:
            for(uint8_t i = 0; i < modbusHandler.qttRegisters; i++){
                uint16_t incomingShort = STRING_GetChar(response, 3 + 2*i) << 8;
                incomingShort |= STRING_GetChar(response, 3 + 2*i + 1);
                uint16_t channel = modbusHandler.firstRegister + i/2;

                if(modbusHandler.firstRegister + i%2 == 0){
                    convertedVoltageReads_V[modbusHandler.requestId - 0x6C + 1][channel] =
                            UTILS_Map(incomingShort,
                                    MIN_ADC_READ, MAX_ADC_READ,
                                    MIN_VOLTAGE_READ, MAX_VOLTAGE_READ);
                }
                else{
                    convertedCurrentReads_mA[modbusHandler.requestId - 0x6C + 1][channel] =
                            UTILS_Map(incomingShort,
                                    MIN_ADC_READ, MAX_ADC_READ,
                                    MIN_CURRENT_READ, MAX_CURRENT_READ);
                }
            }
            break;
        default:
            break;
    }
    STRING_Clear(&modbusLastMessage);
}
// Message treatment functions //

// Uart handling functions // [Section]
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

static void APP_DisableUartInterrupt(UART_HandleTypeDef *huart){
    HAL_UART_Abort_IT(huart);
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

    if (HAL_UART_Init(huart) != HAL_OK){
        Error_Handler();
    }

    HAL_UART_Receive_IT(huart, uartBuffer, 1);
}
// Uart handling functions //

// Specific utility functions // [Section]
static void APP_SendLog(){
    if(!appStarted){
        return;
    }

    string logMessage;
    STRING_Init(&logMessage);
    APP_AddRtcTimestampToString(&logMessage, &hrtc);
    STRING_AddCharString(&logMessage, "\n\r");
    COMM_SendStartPacket();
    COMM_SendAck(LOGS);
    COMM_SendString(&logMessage);

    for(uint16_t i = 0; i < NUMBER_OF_CHANNELS; i++){
        STRING_AddCharString(&logMessage, "\n\r");
        uint8_t line[100] = {'\0'};
        uint8_t length = sprintf((char*)line, "[LOG] Read %d: Voltage = %.2f V ; Current = %.2f mA\n\r", i, convertedVoltageReads_V[0][i], convertedCurrentReads_mA[0][i]);
        COMM_SendChar(line, length);
    }
    COMM_SendEndPacket();
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

void APP_DisableModbus(){
    if(!modbusEnabled)
        return;

    APP_DisableUartInterrupt(MODBUS_UART);

    HAL_GPIO_WritePin(LIGA_RS485_GPIO_Port, LIGA_RS485_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGA_RS485__GPIO_Port, LIGA_RS485__Pin, GPIO_PIN_SET);

    modbusEnabled = 0;
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
    if(!appStarted){
        return;
    }

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
    HAL_StatusTypeDef status;

    if(huart == DISPLAY_UART){
        if(appStarted){
            RB_PutByte(&displayRb, displayLastChar);
        }
        do{
            status = HAL_UART_Receive_IT(DISPLAY_UART, &displayLastChar, 1);
        } while(status != HAL_OK);
    }

    else if(huart == DEBUG_UART){
        debugTimeBetweenByteCounter_ms = 0;
        if(appStarted){
            RB_PutByte(&debugRb, debugLastChar);
        }
        do{
            status = HAL_UART_Receive_IT(DEBUG_UART, &debugLastChar, 1);
        } while(status != HAL_OK);
    }

    else if(huart == MODBUS_UART){
        modbusTimeBetweenByteCounter_ms = 0;
        if(appStarted){
            RB_PutByte(&modbusRb, modbusLastChar);
        }
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
        if(updateDisplayCounter_ms < UPDATE_DISPLAY_PERIOD_MS){
            updateDisplayCounter_ms++;
        }
        if(updateReadsCounter_ms < UPDATE_READS_PERIOD_MS){
            updateReadsCounter_ms++;
        }
        if(modbusTimeBetweenByteCounter_ms < MODBUS_MAX_TIME_BETWEEN_BYTES_MS + modbusMaster*MASTER_BUFFER_PERIOD){
            modbusTimeBetweenByteCounter_ms++;
        }
        if(debugTimeBetweenByteCounter_ms < DEBUG_MAX_TIME_BETWEEN_BYTES_MS){
            debugTimeBetweenByteCounter_ms++;
        }
        if(requestReadsCounter_ms < REQUEST_READS_PERIOD_MS){
            requestReadsCounter_ms++;
        }
        if(testDisplayConnectionCounter_ms < TEST_DISPLAY_CONNECTION_PERIOD_MS){
            requestReadsCounter_ms++;
        }

        vLEDS_LedsTimerCallback();
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc){

}
// Callbacks //
