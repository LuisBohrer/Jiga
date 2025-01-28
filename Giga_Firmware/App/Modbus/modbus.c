/*
 * modbus.c
 *
 *  Created on: Aug 23, 2024
 *      Author: luisv
 */

#include <stdio.h>
#include "modbus.h"
#include "RingBuffer/ringBuffer.h"
#include "crc.h"


// FUNÇOES ESTATICAS //

static inline uint8_t MODBUS_CheckCrc(modbusHandler_t *ModbusHandler);
static inline void MODBUS_ResetHandler(modbusHandler_t *modbusHandler);
static void MODBUS_ResetIndexes(modbusHandler_t *modbusHandler);
static void MODBUS_HandleResponse(modbusHandler_t *modbusHandler);
static void MODBUS_SendByte(modbusHandler_t *modbusHandler, uint8_t byte);
static void MODBUS_SendShort(modbusHandler_t *modbusHandler, uint16_t shortValue);
static void MODBUS_TrataErro(modbusHandler_t *modbusHandler, modbusError_t error);
static void MODBUS_SendCommand(modbusHandler_t *modbusHandler);

// FUNÇOES ESTATICAS //


// VARIAVEIS LOCAIS //

const uint8_t DEVICE_ADDRESS = 0; // relevante apenas se for slave

static uint8_t ui8ModbusEnabled = 0;

uint8_t ui8SendCommandBuffer[MODBUS_BUFFER_SIZE];

ringBuffer_t *stRBModbusRbRx = NULL;
ringBuffer_t *stRBModbusRbTx = NULL;

// VARIAVEIS LOCAIS //

void MODBUS_Init(modbusHandler_t *modbusHandler, GPIO_TypeDef *sendReceivePort, uint16_t sendReceivePin){
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

    HAL_GPIO_WritePin(modbusHandler->sendReceivePort, modbusHandler->sendReceivePin, GPIO_PIN_RESET); // Receive
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

            MODBUS_HandleResponse(modbusHandler);
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
                    MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
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

static void MODBUS_HandleResponse(modbusHandler_t *modbusHandler){
    if(!modbusHandler->timeout_100ms){
        MODBUS_TrataErro(modbusHandler, MODBUS_TIMEOUT);
        return;
    }

    while(!RB_IsEmpty(stRBModbusRbRx)){
        modbusHandler->timeout_100ms = 5; // da mais 500 ms para outro byte chegar

        uint8_t ui8IncomingByte = RB_GetByte(stRBModbusRbRx);

        static uint8_t dataComplete = 0;
        if(modbusHandler->PayloadIndex <= 0){
            if(ui8IncomingByte != modbusHandler->RequestId){
                MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_ID);
                return;
            }

            dataComplete = 0;

            modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
//            modbusHandler->DeviceAddress = ui8IncomingByte;
            continue;
        }

        if(modbusHandler->PayloadIndex <= 1){
            if(ui8IncomingByte != modbusHandler->Opcode){
                if(ui8IncomingByte == modbusHandler->Opcode + 0x80)
                    MODBUS_TrataErro(modbusHandler, MODBUS_RESPONSE_ERROR);
                else
                    MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_OPCODE);
                return;
            }
            modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
            continue;
        }

        switch(modbusHandler->Opcode){
            case READ_HOLDING_REGISTERS:
                if(modbusHandler->PayloadIndex <= 2){
                    modbusHandler->DataSize = ui8IncomingByte;
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->DataIndex < modbusHandler->DataSize){
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    modbusHandler->DataBuffer[modbusHandler->DataIndex++] = ui8IncomingByte;
                    if(modbusHandler->DataIndex % modbusHandler->registerSize == 0){
                        modbusHandler->RegisterReads[(modbusHandler->DataIndex / modbusHandler->registerSize) - 1] = 0;
                        for(uint8_t i = 0; i < modbusHandler->registerSize; i++){
                            modbusHandler->RegisterReads[(modbusHandler->DataIndex / modbusHandler->registerSize) - 1] <<= 8;
                            modbusHandler->RegisterReads[(modbusHandler->DataIndex / modbusHandler->registerSize) - 1] |= modbusHandler->DataBuffer[modbusHandler->DataIndex - modbusHandler->registerSize + i];
                        }
                    }
                    break;
                }

                dataComplete = 1;
                break;

            case WRITE_MULTIPLE_HOLDING_REGISTERS:
                if(modbusHandler->PayloadIndex <= 2){
                    if(ui8IncomingByte != (modbusHandler->FirstRegister >> 8)){
                        MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_FIRST_REGISTER);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->PayloadIndex <= 3){
                    if(ui8IncomingByte != (modbusHandler->FirstRegister & 0xFF)){
                        MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_FIRST_REGISTER);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->PayloadIndex <= 4){
                    if(ui8IncomingByte != (modbusHandler->QttRegisters >> 8)){
                        MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_QTT_REGISTERS);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->PayloadIndex <= 5){
                    if(ui8IncomingByte != (modbusHandler->QttRegisters & 0xFF)){
                        MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_QTT_REGISTERS);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                dataComplete = 1;
                break;

            default:
                MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
                return;
        }
        if(!dataComplete)
            continue;

        static uint8_t ui8CrcFirstByteReceived = 0;
        if(!ui8CrcFirstByteReceived){
            modbusHandler->ExpectedCRC = ui8IncomingByte << 8; // Usando big endian
            ui8CrcFirstByteReceived = 1;
            continue;
        }
        else
            ui8CrcFirstByteReceived = 0;

        modbusHandler->ExpectedCRC |= (ui8IncomingByte); // Usando big endian

        if(MODBUS_CheckCrc(modbusHandler)){
            modbusHandler->ModbusState = MODBUS_PROCESS_MESSAGE;
            MODBUS_ResetIndexes(modbusHandler);
        }
        else{
            MODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_CRC);
            return;
        }
    }
}

static void MODBUS_SendByte(modbusHandler_t *modbusHandler, uint8_t byte){
    RB_PutByte(stRBModbusRbTx, byte);
    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = byte;
}

static void MODBUS_SendShort(modbusHandler_t *modbusHandler, uint16_t shortValue){
    MODBUS_SendByte(modbusHandler, shortValue >> 8);
    MODBUS_SendByte(modbusHandler, shortValue);
}

static inline uint8_t MODBUS_CheckCrc(modbusHandler_t *modbusHandler){
    modbusHandler->CalculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->PayloadBuffer,
            modbusHandler->PayloadIndex);
    modbusHandler->CalculatedCRC = (modbusHandler->CalculatedCRC<<8 & 0xFF00) |
            (modbusHandler->CalculatedCRC>>8 & 0xFF);
    return (modbusHandler->CalculatedCRC == modbusHandler->ExpectedCRC);
}

static void MODBUS_TrataErro(modbusHandler_t *modbusHandler, modbusError_t error){
    static uint16_t ui16FailedRequests = 0;
    ui16FailedRequests++;
    uint8_t errorMessage[15];
    int messageLength = sprintf((char*) errorMessage, "Erro: %d\n", error);
    MODBUS_ResetHandler(modbusHandler);
    RB_ClearBuffer(stRBModbusRbTx);
    RB_ClearBuffer(stRBModbusRbRx);
}

static void MODBUS_SendCommand(modbusHandler_t *modbusHandler){
    if(RB_GetNumberOfBytes(&modbusHandler->SendCommandRingBuffer) < 6){
        MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_MESSAGE);
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
            MODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
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
