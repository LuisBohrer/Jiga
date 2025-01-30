/*
 * modbus.c
 *
 *  Created on: Aug 23, 2024
 *      Author: luisv
 */

#include <stdio.h>
#include <math.h>
#include "modbus.h"
#include "crc.h"
#include "usart.h"


// FUNÇOES ESTATICAS //

static inline void MODBUS_ResetHandler(modbusHandler_t *modbusHandler);
static void MODBUS_ResetIndexes(modbusHandler_t *modbusHandler);
static void MODBUS_SendByte(modbusHandler_t *modbusHandler, uint8_t byte);
static void MODBUS_SendShort(modbusHandler_t *modbusHandler, uint16_t shortValue);
static void MODBUS_SendLong(modbusHandler_t *modbusHandler, uint32_t longValue);
static void inline MODBUS_CalculateCrc(modbusHandler_t *modbusHandler);

// FUNÇOES ESTATICAS //


void MODBUS_Begin(modbusHandler_t *modbusHandler, GPIO_TypeDef *sendReceivePort, uint16_t sendReceivePin, UART_HandleTypeDef *modbusUart, uint8_t deviceAddress){
    modbusHandler->modbusState = MODBUS_STARTING;

    modbusHandler->modbusUart = modbusUart;
    modbusHandler->sendReceivePort = sendReceivePort;
    modbusHandler->sendReceivePin = sendReceivePin;

    modbusHandler->payloadIndex = 0;
    modbusHandler->deviceAddress = deviceAddress;
    modbusHandler->opcode = 0;
    modbusHandler->firstRegister = 0x00;
    modbusHandler->requestId = 0x00;
    modbusHandler->qttRegisters = 0;
    modbusHandler->calculatedCRC = 0;

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);

    modbusHandler->modbusState = MODBUS_IDLE;
}

static void MODBUS_ResetHandler(modbusHandler_t *modbusHandler){
    modbusHandler->modbusState = MODBUS_STARTING;
    modbusHandler->payloadIndex = 0;
    modbusHandler->requestId = 0x00;
    modbusHandler->opcode = 0;
    modbusHandler->firstRegister = 0x00;
    modbusHandler->qttRegisters = 0;
    modbusHandler->calculatedCRC = 0;
    modbusHandler->modbusState = MODBUS_IDLE;
}

static void MODBUS_ResetIndexes(modbusHandler_t *modbusHandler){
    modbusHandler->payloadIndex = 0;
}

static void MODBUS_SendByte(modbusHandler_t *modbusHandler, uint8_t byte){
    HAL_UART_Transmit(modbusHandler->modbusUart, &byte, 1, 100); // talvez tenha problema com o &byte [TEST]
    modbusHandler->payloadBuffer[modbusHandler->payloadIndex++] = byte;
}

static void MODBUS_SendShort(modbusHandler_t *modbusHandler, uint16_t shortValue){
    MODBUS_SendByte(modbusHandler, shortValue >> 8);
    MODBUS_SendByte(modbusHandler, shortValue);
}

static void MODBUS_SendLong(modbusHandler_t *modbusHandler, uint32_t longValue){
    MODBUS_SendByte(modbusHandler, longValue >> 24);
    MODBUS_SendByte(modbusHandler, longValue >> 16);
    MODBUS_SendByte(modbusHandler, longValue >> 8);
    MODBUS_SendByte(modbusHandler, longValue);
}

static inline void MODBUS_CalculateCrc(modbusHandler_t *modbusHandler){
    modbusHandler->calculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->payloadBuffer, modbusHandler->payloadIndex);
}

void MODBUS_SetSendReceive(modbusHandler_t *modbusHandler, sendOrReceive_t sendOrReceive){
    HAL_GPIO_WritePin(modbusHandler->sendReceivePort, modbusHandler->sendReceivePin, sendOrReceive);
    if(sendOrReceive == MODBUS_SET_RECEIVE){
        modbusHandler->modbusState = MODBUS_RECEIVING;
    }
    else{
        modbusHandler->modbusState = MODBUS_SENDING;
    }
}

sendOrReceive_t MODBUS_GetSendReceive(modbusHandler_t *modbusHandler){
    if(modbusHandler->modbusState == MODBUS_SENDING)
        return MODBUS_SENDING;
    return MODBUS_RECEIVING;
}

modbusError_t MODBUS_VerifyMessage(uint8_t expectedSecondaryAddress, uint8_t expectedOpcode, uint16_t expectedFirstAdress, uint16_t expectedNumberOfData, uint8_t *messageBuffer, uint32_t messageLength){
    if(messageLength < 6){
        return MODBUS_INVALID_MESSAGE;
    }
    if(expectedSecondaryAddress != messageBuffer[0]){
        return MODBUS_INCORRECT_ID;
    }
    if(expectedOpcode == (messageBuffer[1] - 0x80)){
        return MODBUS_RESPONSE_ERROR;
    }
    if(expectedOpcode != messageBuffer[1]){
        return MODBUS_INCORRECT_OPCODE;
    }
    if(expectedFirstAdress != ((messageBuffer[2] << 8) | messageBuffer[3])){
        return MODBUS_INCORRECT_FIRST_REGISTER;
    }
    if(expectedNumberOfData != ((messageBuffer[4] << 8) | messageBuffer[5])){
        return MODBUS_INCORRECT_QTT_REGISTERS;
    }
    uint16_t calculatedCrc = HAL_CRC_Calculate(&hcrc, (uint32_t*) messageBuffer, messageLength - 2);
    if(calculatedCrc != ((messageBuffer[messageLength - 2] << 8) | messageBuffer[messageLength - 1])){
        return MODBUS_INCORRECT_CRC;
    }
    return MODBUS_NO_ERROR;
}

modbusError_t MODBUS_VerifyWithHandler(modbusHandler_t *modbusHandler, uint8_t *messageBuffer, uint32_t messageLength){
    if(messageLength < 6){
        return MODBUS_INVALID_MESSAGE;
    }
    if(modbusHandler->requestId != messageBuffer[0]){
        return MODBUS_INCORRECT_ID;
    }
    if(modbusHandler->opcode == (messageBuffer[1] - 0x80)){
        return MODBUS_RESPONSE_ERROR;
    }
    if(modbusHandler->opcode != messageBuffer[1]){
        return MODBUS_INCORRECT_OPCODE;
    }
    if(modbusHandler->firstRegister != ((messageBuffer[2] << 8) | messageBuffer[3])){
        return MODBUS_INCORRECT_FIRST_REGISTER;
    }
    if(modbusHandler->qttRegisters != ((messageBuffer[4] << 8) | messageBuffer[5])){
        return MODBUS_INCORRECT_QTT_REGISTERS;
    }
    uint16_t calculatedCrc = HAL_CRC_Calculate(&hcrc, (uint32_t*) messageBuffer, messageLength - 2);
    if(calculatedCrc != ((messageBuffer[messageLength - 2] << 8) | messageBuffer[messageLength - 1])){
        return MODBUS_INCORRECT_CRC;
    }
    return MODBUS_NO_ERROR;
}

modbusError_t MODBUS_VerifyCrc(uint8_t *message, uint32_t length){
    if(length < 6){
        return MODBUS_INVALID_MESSAGE;
    }
    if(HAL_CRC_Calculate(&hcrc, (uint32_t*) message, length - 2) != ((message[length - 2] << 8) | message[length - 1] )){
        return MODBUS_INCORRECT_CRC;
    }
    return MODBUS_NO_ERROR;
}

void MODBUS_ReadCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t coilAddress, uint16_t numberOfCoils){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, READ_COILS);
    MODBUS_SendShort(modbusHandler, coilAddress);
    MODBUS_SendShort(modbusHandler, numberOfCoils);

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_ReadInputRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress, uint16_t numberOfRegisters){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, READ_INPUT_REGISTERS);
    MODBUS_SendShort(modbusHandler, registerAddress);
    MODBUS_SendShort(modbusHandler, numberOfRegisters);

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_ReadHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, READ_HOLDING_REGISTERS);
    MODBUS_SendShort(modbusHandler, firstRegisterAddress);
    MODBUS_SendShort(modbusHandler, numberOfRegisters);

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_WriteSingleCoil(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t coilAddress, uint8_t valueToWrite){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, WRITE_SINGLE_COIL);
    MODBUS_SendShort(modbusHandler, coilAddress);
    if(valueToWrite == 0){
        MODBUS_SendShort(modbusHandler, 0x0000);
    }
    else{
        MODBUS_SendShort(modbusHandler, 0xFF00);
    }

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_WriteMultipleCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstCoilAddress, uint16_t numberOfCoils, uint8_t *valuesToWrite){
    uint32_t numberOfBytesToWrite = numberOfCoils/8;
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, WRITE_MULTIPLE_COILS);
    MODBUS_SendShort(modbusHandler, firstCoilAddress);
    MODBUS_SendShort(modbusHandler, numberOfCoils);
    MODBUS_SendByte(modbusHandler, numberOfBytesToWrite);
    for(uint32_t i = 0; i < numberOfBytesToWrite; i++){
        MODBUS_SendByte(modbusHandler, valuesToWrite[i]);
    }

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_WriteSingleHoldingRegister(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress, uint32_t valueToWrite, registerBytes_t sizeOfRegisterBytes){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, WRITE_SINGLE_HOLDING_REGISTER);
    MODBUS_SendShort(modbusHandler, registerAddress);
    if(sizeOfRegisterBytes == MODBUS_REGISTER_16_BITS){
        MODBUS_SendShort(modbusHandler, valueToWrite);
    }
    else if(sizeOfRegisterBytes == MODBUS_REGISTER_32_BITS){
        MODBUS_SendLong(modbusHandler, valueToWrite);
    }

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_WriteMultipleHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters, registerBytes_t sizeOfRegisterBytes, uint8_t* valuesToWrite){
    uint32_t numberOfBytesToWrite = numberOfRegisters*((uint8_t)sizeOfRegisterBytes);
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, WRITE_MULTIPLE_HOLDING_REGISTERS);
    MODBUS_SendShort(modbusHandler, firstRegisterAddress);
    MODBUS_SendShort(modbusHandler, numberOfRegisters);
    MODBUS_SendByte(modbusHandler, numberOfBytesToWrite);
    for(uint32_t i = 0; i < numberOfBytesToWrite; i++){
        MODBUS_SendByte(modbusHandler, valuesToWrite[i]);
    }

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->calculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}
