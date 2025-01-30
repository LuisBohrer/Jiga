/*
 * modbus.c
 *
 *  Created on: Aug 23, 2024
 *      Author: luisv
 */

#include <stdio.h>
#include <math.h>
#include "modbus.h"
#include "RingBuffer/ringBuffer.h"
#include "crc.h"
#include "usart.h"


// FUNÇOES ESTATICAS //

static inline uint8_t MODBUS_CheckCrc(modbusHandler_t *ModbusHandler);
static inline void MODBUS_ResetHandler(modbusHandler_t *modbusHandler);
static void MODBUS_ResetIndexes(modbusHandler_t *modbusHandler);
static void MODBUS_SendByte(modbusHandler_t *modbusHandler, uint8_t byte);
static void MODBUS_SendShort(modbusHandler_t *modbusHandler, uint16_t shortValue);
static void MODBUS_SendLong(modbusHandler_t *modbusHandler, uint32_t longValue);
static void MODBUS_SendCommand(modbusHandler_t *modbusHandler);
static void inline MODBUS_CalculateCrc(modbusHandler_t *modbusHandler);

// FUNÇOES ESTATICAS //


// VARIAVEIS LOCAIS //

UART_HandleTypeDef *modbusUart;

const uint8_t DEVICE_ADDRESS = 0; // relevante apenas se for slave

static uint8_t ui8ModbusEnabled = 0;

uint8_t ui8SendCommandBuffer[MODBUS_BUFFER_SIZE];

ringBuffer_t *stRBModbusRbRx = NULL;
ringBuffer_t *stRBModbusRbTx = NULL;

// VARIAVEIS LOCAIS //

void MODBUS_Init(modbusHandler_t *modbusHandler, GPIO_TypeDef *sendReceivePort, uint16_t sendReceivePin, UART_HandleTypeDef *huart){
    modbusUart = huart;

    modbusHandler->sendReceivePort = sendReceivePort;
    modbusHandler->sendReceivePin = sendReceivePin;

    modbusHandler->ModbusState = MODBUS_IDLE;
    RB_Init(&modbusHandler->SendCommandRingBuffer);
    modbusHandler->PayloadIndex = 0;
    modbusHandler->DeviceAddress = DEVICE_ADDRESS;
    modbusHandler->Opcode = 0;
    modbusHandler->FirstRegister = 0x00;
    modbusHandler->RequestId = 0x00;
    modbusHandler->QttRegisters = 0;
    modbusHandler->DataIndex = 0;
    modbusHandler->DataComplete = 0;
    modbusHandler->ExpectedCRC = 0;
    modbusHandler->CalculatedCRC = 0;
    modbusHandler->timeout_100ms = 0;
    modbusHandler->registerSize = 2;

//    stRBModbusRbRx = vAPP_GetUartRbRx(EN_UART_MODBUS);
//    stRBModbusRbTx = vAPP_GetUartRbTx(EN_UART_MODBUS);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

static void MODBUS_ResetHandler(modbusHandler_t *modbusHandler){
    modbusHandler->ModbusState = MODBUS_IDLE;
    modbusHandler->PayloadIndex = 0;
    modbusHandler->DeviceAddress = DEVICE_ADDRESS;
    modbusHandler->RequestId = 0x00;
    modbusHandler->Opcode = 0;
    modbusHandler->FirstRegister = 0x00;
    modbusHandler->QttRegisters = 0;
    modbusHandler->DataIndex = 0;
    modbusHandler->DataComplete = 0;
    modbusHandler->ExpectedCRC = 0;
    modbusHandler->CalculatedCRC = 0;
    modbusHandler->timeout_100ms = 1; // Aguarda 100 ms antes de tentar de novo
}

static void MODBUS_ResetIndexes(modbusHandler_t *modbusHandler){
    modbusHandler->PayloadIndex = 0;
    modbusHandler->DataIndex = 0;
}

void vMODBUS_Poll(modbusHandler_t *modbusHandler){
    if(!ui8ModbusEnabled){
        return;
    }

    switch(modbusHandler->ModbusState){
        case MODBUS_IDLE:
            //			if(modbusHandler->timeout_100ms)
            //				return;

            if(!RB_IsEmpty(&modbusHandler->SendCommandRingBuffer)){
                modbusHandler->ModbusState = MODBUS_SENDING;
                modbusHandler->timeout_100ms = 2;
            }

            else if(!RB_IsEmpty(stRBModbusRbRx)){
                modbusHandler->ModbusState = MODBUS_RECEIVING;
                modbusHandler->timeout_100ms = 5;
            }
            break;

        case MODBUS_SENDING:
            MODBUS_SendCommand(modbusHandler);
            break;

        case MODBUS_RECEIVING:
            HAL_GPIO_WritePin(E_RS485_GPIO_Port, E_RS485_Pin, GPIO_PIN_RESET); // Receive

//            MODBUS_HandleResponse(modbusHandler);
            break;

        case MODBUS_PROCESS_MESSAGE:
            static uint16_t ui16successfulRequests = 0;
            ui16successfulRequests++;

            switch(modbusHandler->Opcode){
                case READ_HOLDING_REGISTERS:
                    uint8_t message[50];
                    uint8_t messageLength = 0;
                    for(uint8_t i = 0; i < modbusHandler->QttRegisters; i++){
                        messageLength = sprintf((char*)message, "Registrador %d: %lud\n",
                                modbusHandler->FirstRegister + i,
                                modbusHandler->RegisterReads[i]);
                    }
                    break;

                default:
//                    MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
                    break;
            }

            modbusHandler->timeout_100ms = 2;
            modbusHandler->ModbusState = MODBUS_IDLE;
            break;

        default:
            MODBUS_ResetHandler(modbusHandler);
            break;
    }
}

static void MODBUS_SendByte(modbusHandler_t *modbusHandler, uint8_t byte){
//    RB_PutByte(stRBModbusRbTx, byte);
    HAL_UART_Transmit(modbusUart, &byte, 1, 100); // talvez tenha problema com o &byte [TEST]
    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = byte;
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
    modbusHandler->CalculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->PayloadBuffer, modbusHandler->PayloadIndex);
}

static inline uint8_t MODBUS_CheckCrc(modbusHandler_t *modbusHandler){
    modbusHandler->CalculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->PayloadBuffer,
            modbusHandler->PayloadIndex);
    modbusHandler->CalculatedCRC = (modbusHandler->CalculatedCRC<<8 & 0xFF00) |
            (modbusHandler->CalculatedCRC>>8 & 0xFF);
    return (modbusHandler->CalculatedCRC == modbusHandler->ExpectedCRC);
}

static void MODBUS_SendCommand(modbusHandler_t *modbusHandler){
    if(RB_GetNumberOfBytes(&modbusHandler->SendCommandRingBuffer) < 6){
//        MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_MESSAGE);
        return;
    }

    HAL_GPIO_WritePin(E_RS485_GPIO_Port, E_RS485_Pin, GPIO_PIN_SET); // Send

    modbusHandler->RequestId = RB_GetByte(&modbusHandler->SendCommandRingBuffer);
    modbusHandler->Opcode = RB_GetByte(&modbusHandler->SendCommandRingBuffer);
    modbusHandler->FirstRegister = RB_GetByte(&modbusHandler->SendCommandRingBuffer) << 8;
    modbusHandler->FirstRegister |= RB_GetByte(&modbusHandler->SendCommandRingBuffer);
    modbusHandler->QttRegisters = RB_GetByte(&modbusHandler->SendCommandRingBuffer) << 8;
    modbusHandler->QttRegisters |= RB_GetByte(&modbusHandler->SendCommandRingBuffer);
    modbusHandler->QttBytes = RB_GetByte(&modbusHandler->SendCommandRingBuffer);
    modbusHandler->registerSize = modbusHandler->QttBytes/modbusHandler->QttRegisters;

    if(modbusHandler->Opcode == WRITE_SINGLE_COIL ||
            modbusHandler->Opcode == WRITE_MULTIPLE_COILS ||
            modbusHandler->Opcode == WRITE_SINGLE_HOLDING_REGISTER ||
            modbusHandler->Opcode == WRITE_MULTIPLE_HOLDING_REGISTERS){
        for(uint8_t i = 0; i < modbusHandler->QttBytes; i++){
            modbusHandler->DataBuffer[i] = RB_GetByte(&modbusHandler->SendCommandRingBuffer);
        }
    }

    MODBUS_SendByte(modbusHandler, modbusHandler->RequestId);
    MODBUS_SendByte(modbusHandler, modbusHandler->Opcode);
    MODBUS_SendShort(modbusHandler, modbusHandler->FirstRegister);
    switch(modbusHandler->Opcode){
        case READ_HOLDING_REGISTERS:
            MODBUS_SendShort(modbusHandler, modbusHandler->QttRegisters);
            break;

        case WRITE_SINGLE_COIL:
            if(modbusHandler->DataBuffer[0] == 0)
                MODBUS_SendShort(modbusHandler, 0x0000);
            else
                MODBUS_SendShort(modbusHandler, 0xFF00);
            break;

        case WRITE_MULTIPLE_COILS: // talvez precise corrigir
            MODBUS_SendShort(modbusHandler, modbusHandler->QttRegisters);

            for(uint8_t i = 0; i < modbusHandler->QttBytes; i ++){
                MODBUS_SendByte(modbusHandler, modbusHandler->DataBuffer[i]);
            }
            break;

        case WRITE_SINGLE_HOLDING_REGISTER:
            for(uint8_t i = 0; i < modbusHandler->QttBytes; i++){
                MODBUS_SendByte(modbusHandler, modbusHandler->DataBuffer[i]);
            }
            break;

        case WRITE_MULTIPLE_HOLDING_REGISTERS:
            MODBUS_SendShort(modbusHandler, modbusHandler->QttRegisters);

            MODBUS_SendByte(modbusHandler, modbusHandler->QttBytes);
            for(uint8_t i = 0; i < modbusHandler->QttBytes; i++){
                MODBUS_SendByte(modbusHandler, modbusHandler->DataBuffer[i]);
            }
            break;

        default:
//            MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
            return;
    }

    modbusHandler->CalculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->PayloadBuffer,
            modbusHandler->PayloadIndex);
    modbusHandler->CalculatedCRC = (modbusHandler->CalculatedCRC<<8 & 0xFF00) |
            (modbusHandler->CalculatedCRC>>8 & 0xFF);
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

    modbusHandler->timeout_100ms = 2; // aguarda 200 ms
    modbusHandler->ModbusState = MODBUS_RECEIVING;
    MODBUS_ResetIndexes(modbusHandler);
}

void MODBUS_SetSendReceive(modbusHandler_t *modbusHandler, sendOrReceive_t sendOrReceive){
    HAL_GPIO_WritePin(modbusHandler->sendReceivePort, modbusHandler->sendReceivePin, sendOrReceive);
    if(sendOrReceive == MODBUS_SET_RECEIVE){
        modbusHandler->ModbusState = MODBUS_RECEIVING;
    }
    else{
        modbusHandler->ModbusState = MODBUS_SENDING;
    }
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
    if(modbusHandler->RequestId != messageBuffer[0]){
        return MODBUS_INCORRECT_ID;
    }
    if(modbusHandler->Opcode == (messageBuffer[1] - 0x80)){
        return MODBUS_RESPONSE_ERROR;
    }
    if(modbusHandler->Opcode != messageBuffer[1]){
        return MODBUS_INCORRECT_OPCODE;
    }
    if(modbusHandler->FirstRegister != ((messageBuffer[2] << 8) | messageBuffer[3])){
        return MODBUS_INCORRECT_FIRST_REGISTER;
    }
    if(modbusHandler->QttRegisters != ((messageBuffer[4] << 8) | messageBuffer[5])){
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

void vMODBUS_Read(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint8_t command, uint16_t offset, uint16_t numberOfRegisters, registerBytes_t sizeOfRegistersBytes){
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, secondaryAddress);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, command);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, offset >> 8);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, offset & 0xFF);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters >> 8);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters & 0xFF);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters*sizeOfRegistersBytes);
}

void vMODBUS_Write(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint8_t command, uint16_t offset, uint16_t numberOfRegisters, registerBytes_t sizeOfRegistersBytes, uint16_t* Parameters){
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, secondaryAddress);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, command);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, offset >> 8);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, offset & 0xFF);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters >> 8);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters & 0xFF);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters*sizeOfRegistersBytes);
    for(uint8_t i = 0; i < sizeOfRegistersBytes; i++){
        RB_PutByte(&modbusHandler->SendCommandRingBuffer,
                Parameters[i/sizeOfRegistersBytes] >> (8*((sizeOfRegistersBytes - 1) - i%sizeOfRegistersBytes)));
    }
}

void MODBUS_ReadCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t coilAddress, uint16_t numberOfCoils){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, READ_COILS);
    MODBUS_SendShort(modbusHandler, coilAddress);
    MODBUS_SendShort(modbusHandler, numberOfCoils);

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}

void MODBUS_ReadInputRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress);

void MODBUS_ReadHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters){
    MODBUS_ResetIndexes(modbusHandler);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_SEND);

    MODBUS_SendByte(modbusHandler, secondaryAddress);
    MODBUS_SendByte(modbusHandler, READ_HOLDING_REGISTERS);
    MODBUS_SendShort(modbusHandler, firstRegisterAddress);
    MODBUS_SendShort(modbusHandler, numberOfRegisters);

    MODBUS_CalculateCrc(modbusHandler);
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

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
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

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
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

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
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

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
    MODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

    MODBUS_SetSendReceive(modbusHandler, MODBUS_SET_RECEIVE);
}
