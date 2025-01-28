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

static inline uint8_t sui8MODBUS_CheckCrc(MODBUS_ModbusHandler_t *ModbusHandler);
static inline void svMODBUS_ResetHandler(MODBUS_ModbusHandler_t *modbusHandler);
static void svMODBUS_ResetIndexes(MODBUS_ModbusHandler_t *modbusHandler);
//static uint16_t suiMB_Calc_CRC(uint8_t *pucByte, uint16_t uiLength);
static void svMODBUS_Handle_Response(MODBUS_ModbusHandler_t *modbusHandler);
static void svMODBUS_SendByte(MODBUS_ModbusHandler_t *modbusHandler, uint8_t byte);
static void svMODBUS_SendShort(MODBUS_ModbusHandler_t *modbusHandler, uint16_t shortValue);
static void svMODBUS_TrataErro(MODBUS_ModbusHandler_t *modbusHandler, enMODBUS_Error_t error);
static void svMODBUS_SendCommand(MODBUS_ModbusHandler_t *modbusHandler);

// FUNÇOES ESTATICAS //


// VARIAVEIS LOCAIS //

const uint8_t DEVICE_ADDRESS = 0; // relevante apenas se for slave

static uint8_t ui8ModbusEnabled = 0;

uint8_t ui8SendCommandBuffer[MODBUS_BUFFER_SIZE];

ringBuffer_t *stRBModbusRbRx = NULL;
ringBuffer_t *stRBModbusRbTx = NULL;

// VARIAVEIS LOCAIS //

// TODO -> modularizar o enable e disable
void vMODBUS_EnableModbus(){
    if(ui8ModbusEnabled)
        return;

    HAL_GPIO_WritePin(LIGA_RS485_GPIO_Port, LIGA_RS485_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LIGA_RS485__GPIO_Port, LIGA_RS485__Pin, GPIO_PIN_RESET);

    while(ui8APP_EnableUartInterrupt(APP_GetUart(EN_UART_MODBUS)) != TRUE);

    vRB_ClearBuffer(stRBModbusRbRx);
    vRB_ClearBuffer(stRBModbusRbTx);

    ui8ModbusEnabled = TRUE;
}

void vMODBUS_DisableModbus(){
    if(!ui8ModbusEnabled)
        return;

    vAPP_DisableUartInterrupt(APP_GetUart(EN_UART_MODBUS));

    HAL_GPIO_WritePin(LIGA_RS485_GPIO_Port, LIGA_RS485_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGA_RS485__GPIO_Port, LIGA_RS485__Pin, GPIO_PIN_SET);

    ui8ModbusEnabled = FALSE;
}

void vMODBUS_Init(MODBUS_ModbusHandler_t *modbusHandler){
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

    stRBModbusRbRx = vAPP_GetUartRbRx(EN_UART_MODBUS);
    stRBModbusRbTx = vAPP_GetUartRbTx(EN_UART_MODBUS);

    HAL_GPIO_WritePin(E_RS485_GPIO_Port, E_RS485_Pin, GPIO_PIN_SET); // Send
    //	HAL_GPIO_WritePin(E_RS485_GPIO_Port, E_RS485_Pin, GPIO_PIN_RESET); // Receive
}

static void svMODBUS_ResetHandler(MODBUS_ModbusHandler_t *modbusHandler){
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

static void svMODBUS_ResetIndexes(MODBUS_ModbusHandler_t *modbusHandler){
    modbusHandler->PayloadIndex = 0;
    modbusHandler->DataIndex = 0;
}

void vMODBUS_Poll(MODBUS_ModbusHandler_t *modbusHandler){
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
            svMODBUS_SendCommand(modbusHandler);
            break;

        case MODBUS_RECEIVING:
            HAL_GPIO_WritePin(E_RS485_GPIO_Port, E_RS485_Pin, GPIO_PIN_RESET); // Receive

            svMODBUS_Handle_Response(modbusHandler);
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
                        vDISPLAY_SendLog(message, messageLength, LOGS_MODBUS);

                        vDB_storeShort(MODBUS_REGISTRADOR_0_DB_ADDRESS
                                + modbusHandler->FirstRegister + i,
                                modbusHandler->RegisterReads[i]);
                    }

                    vDB_storeShort(MODBUS_ACK_REQ_DB_ADDRESS, ui16successfulRequests);
                    break;

                default:
                    svMODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
                    break;
            }

            modbusHandler->timeout_100ms = 2;
            modbusHandler->ModbusState = MODBUS_IDLE;
            break;

        default:
            svMODBUS_ResetHandler(modbusHandler);
            break;
    }
}

static void svMODBUS_Handle_Response(MODBUS_ModbusHandler_t *modbusHandler){
    if(!modbusHandler->timeout_100ms){
        svMODBUS_TrataErro(modbusHandler, MODBUS_TIMEOUT);
        return;
    }

    while(!RB_IsEmpty(stRBModbusRbRx)){
        modbusHandler->timeout_100ms = 5; // da mais 500 ms para outro byte chegar

        uint8_t ui8IncomingByte = RB_GetByte(stRBModbusRbRx);

        static uint8_t dataComplete = 0;
        if(modbusHandler->PayloadIndex <= 0){
            if(ui8IncomingByte != modbusHandler->RequestId){
                svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_ID);
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
                    svMODBUS_TrataErro(modbusHandler, MODBUS_RESPONSE_ERROR);
                else
                    svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_OPCODE);
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
                        svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_FIRST_REGISTER);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->PayloadIndex <= 3){
                    if(ui8IncomingByte != (modbusHandler->FirstRegister & 0xFF)){
                        svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_FIRST_REGISTER);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->PayloadIndex <= 4){
                    if(ui8IncomingByte != (modbusHandler->QttRegisters >> 8)){
                        svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_QTT_REGISTERS);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                if(modbusHandler->PayloadIndex <= 5){
                    if(ui8IncomingByte != (modbusHandler->QttRegisters & 0xFF)){
                        svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_QTT_REGISTERS);
                        return;
                    }
                    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = ui8IncomingByte;
                    break;
                }

                dataComplete = 1;
                break;

            default:
                svMODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
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

        if(sui8MODBUS_CheckCrc(modbusHandler)){
            modbusHandler->ModbusState = MODBUS_PROCESS_MESSAGE;
            svMODBUS_ResetIndexes(modbusHandler);
        }
        else{
            svMODBUS_TrataErro(modbusHandler, MODBUS_INCORRECT_CRC);
            return;
        }
    }
}

static void svMODBUS_SendByte(MODBUS_ModbusHandler_t *modbusHandler, uint8_t byte){
    RB_PutByte(stRBModbusRbTx, byte);
    modbusHandler->PayloadBuffer[modbusHandler->PayloadIndex++] = byte;
}

static void svMODBUS_SendShort(MODBUS_ModbusHandler_t *modbusHandler, uint16_t shortValue){
    svMODBUS_SendByte(modbusHandler, shortValue >> 8);
    svMODBUS_SendByte(modbusHandler, shortValue);
}

//#define MB_CRC_POLYNOMIAL 0xA001
//// Funcao base de calculo do CRC
//static void svMB_CrcFunction(uint16_t *uiCrc, uint8_t ucData)
//{
//    register uint8_t i;
//    *uiCrc ^= ucData;
//    for (i = 0; i < 8; i++) {
//        if (!(*uiCrc & 1))
//            *uiCrc >>= 1;
//        else {
//            *uiCrc >>= 1;
//            *uiCrc ^= MB_CRC_POLYNOMIAL;
//        }
//    }
//}

//static uint16_t suiMB_Calc_CRC(uint8_t *pucByte, uint16_t uiLength)
//{
//    uint16_t uiCRC = 0xFFFF; // Valor inicial de calculo do CRC
//
//    while (uiLength--) {
//        svMB_CrcFunction(&uiCRC, *pucByte++);
//    }
//    return (uiCRC << 8) | (uiCRC >> 8);
//}

static inline uint8_t sui8MODBUS_CheckCrc(MODBUS_ModbusHandler_t *modbusHandler){
    modbusHandler->CalculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->PayloadBuffer,
            modbusHandler->PayloadIndex);
    modbusHandler->CalculatedCRC = (modbusHandler->CalculatedCRC<<8 & 0xFF00) |
            (modbusHandler->CalculatedCRC>>8 & 0xFF);
    return (modbusHandler->CalculatedCRC == modbusHandler->ExpectedCRC);
}

static void svMODBUS_TrataErro(MODBUS_ModbusHandler_t *modbusHandler, enMODBUS_Error_t error){
    static uint16_t ui16FailedRequests = 0;
    ui16FailedRequests++;
    vDB_storeShort(MODBUS_NACK_REQ_DB_ADDRESS, ui16FailedRequests);

    vDB_storeByte(ERRO_MODBUS_DB_ADDRESS, error);
    uint8_t errorMessage[15];
    int messageLength = sprintf((char*) errorMessage, "Erro: %d\n", error);
    vDISPLAY_SendLog(errorMessage, messageLength, LOGS_MODBUS);
    svMODBUS_ResetHandler(modbusHandler);
    vRB_ClearBuffer(stRBModbusRbTx);
    vRB_ClearBuffer(stRBModbusRbRx);
}

static void svMODBUS_SendCommand(MODBUS_ModbusHandler_t *modbusHandler){
    if(RB_GetNumberOfBytes(&modbusHandler->SendCommandRingBuffer) < 6){
        svMODBUS_TrataErro(modbusHandler, MODBUS_INVALID_MESSAGE);
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

    svMODBUS_SendByte(modbusHandler, modbusHandler->RequestId);
    svMODBUS_SendByte(modbusHandler, modbusHandler->Opcode);
    svMODBUS_SendShort(modbusHandler, modbusHandler->FirstRegister);
    switch(modbusHandler->Opcode){
        case READ_HOLDING_REGISTERS:
            svMODBUS_SendShort(modbusHandler, modbusHandler->QttRegisters);
            break;

        case WRITE_SINGLE_COIL:
            if(modbusHandler->DataBuffer[0] == 0)
                svMODBUS_SendShort(modbusHandler, 0x0000);
            else
                svMODBUS_SendShort(modbusHandler, 0xFF00);
            break;

        case WRITE_MULTIPLE_COILS: // talvez precise corrigir
            svMODBUS_SendShort(modbusHandler, modbusHandler->QttRegisters);

            for(uint8_t i = 0; i < modbusHandler->QttBytes; i ++){
                svMODBUS_SendByte(modbusHandler, modbusHandler->DataBuffer[i]);
            }
            break;

        case WRITE_SINGLE_HOLDING_REGISTER:
            for(uint8_t i = 0; i < modbusHandler->QttBytes; i++){
                svMODBUS_SendByte(modbusHandler, modbusHandler->DataBuffer[i]);
            }
            break;

        case WRITE_MULTIPLE_HOLDING_REGISTERS:
            svMODBUS_SendShort(modbusHandler, modbusHandler->QttRegisters);

            svMODBUS_SendByte(modbusHandler, modbusHandler->QttBytes);
            for(uint8_t i = 0; i < modbusHandler->QttBytes; i++){
                svMODBUS_SendByte(modbusHandler, modbusHandler->DataBuffer[i]);
            }
            break;

        default:
            svMODBUS_TrataErro(modbusHandler, MODBUS_INVALID_OPCODE);
            return;
    }

    modbusHandler->CalculatedCRC = (uint16_t) HAL_CRC_Calculate(&hcrc,
            (uint32_t*)modbusHandler->PayloadBuffer,
            modbusHandler->PayloadIndex);
    modbusHandler->CalculatedCRC = (modbusHandler->CalculatedCRC<<8 & 0xFF00) |
            (modbusHandler->CalculatedCRC>>8 & 0xFF);
    svMODBUS_SendShort(modbusHandler, modbusHandler->CalculatedCRC);

    vUART_Poll();

    modbusHandler->timeout_100ms = 2; // aguarda 200 ms
    modbusHandler->ModbusState = MODBUS_RECEIVING;
    svMODBUS_ResetIndexes(modbusHandler);
}

void vMODBUS_Read(MODBUS_ModbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint8_t command, uint16_t offset, uint16_t numberOfRegisters, enMODBUS_RegisterBytes_t sizeOfRegistersBytes){
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, secondaryAddress);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, command);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, offset >> 8);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, offset & 0xFF);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters >> 8);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters & 0xFF);
    RB_PutByte(&modbusHandler->SendCommandRingBuffer, numberOfRegisters*sizeOfRegistersBytes);
}

void vMODBUS_Write(MODBUS_ModbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint8_t command, uint16_t offset, uint16_t numberOfRegisters, enMODBUS_RegisterBytes_t sizeOfRegistersBytes, uint16_t* Parameters){
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
