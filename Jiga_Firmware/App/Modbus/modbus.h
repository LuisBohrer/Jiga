/*
 * modbus.h
 *
 *  Created on: Aug 23, 2024
 *      Author: luisv
 */

#ifndef MODBUS_MODBUS_H_
#define MODBUS_MODBUS_H_

// INCLUDES //

#include "gpio.h"
#include "usart.h"

// INCLUDES //


// DEFINES / CONSTANTES //

#define MODBUS_BUFFER_SIZE 500
#define MODBUS_FIRST_ADDRESS 0xC8
#define MODBUS_MAX_REGISTER 10

// DEFINES / CONSTANTES //


// ENUMS //

typedef enum enMODBUS_ModbusStates{
    MODBUS_STARTING = 0,
    MODBUS_IDLE,
    MODBUS_SENDING,
    MODBUS_RECEIVING,
} modbusStates_t;

typedef enum enMODBUS_Opcodes{
    READ_COILS = 0x01,
    READ_DISCRETE_INPUTS,
    READ_HOLDING_REGISTERS,
    READ_INPUT_REGISTERS,
    WRITE_SINGLE_COIL,
    WRITE_SINGLE_HOLDING_REGISTER,
    READ_EXCEPTION_STATUS,
    DIAGNOSTICS,
    GET_COMM_EVENT_COUNTER = 0x0B,
    GET_COMM_EVENT_LOG,
    WRITE_MULTIPLE_COILS = 0x0F,
    WRITE_MULTIPLE_HOLDING_REGISTERS,
    REPORT_SLAVE_ID,
    READ_FILE_RECORD = 0x14,
    WRITE_FILE_RECORD,
    MASK_WRITE_REGISTER,
    READ_WRITE_MULTIPLE_REGISTER,
    READ_FIFO_QUEUE,
    ENCAPSULATED_INTERFACE_TRANSPORT = 0x2B
} modbusOpcodes_t;

typedef enum enMODBUS_Error{
    MODBUS_NO_ERROR = 0,
    MODBUS_ILLEGAL_FUNCTION,
    MODBUS_ILLEGAL_DATA_ADDRESS,
    MODBUS_ILLEGAL_DATA_VALUE,
    MODBUS_SLAVE_DEVICE_FAILURE,
    MODBUS_ACKNOWLEDGE,
    MODBUS_SLAVE_DEVICE_BUSY,
    MODBUS_NEGATIVE_ACKNOWLEDGE,
    MODBUS_MEMORY_PARITY,
    MODBUS_GATEWAY_PATH_UNAVAILABLE = 0x0A,
    MODBUS_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND,
    MODBUS_TIMEOUT = 0x67,
    MODBUS_INCORRECT_ID = 0X91,
    MODBUS_INCORRECT_OPCODE,
    MODBUS_INCORRECT_FIRST_REGISTER,
    MODBUS_INCORRECT_QTT_REGISTERS,
    MODBUS_RESPONSE_ERROR,
    MODBUS_INCORRECT_CRC = 0x96,
    MODBUS_INCOMPLETE_MESSAGE = 0xFF,
} modbusError_t;

typedef enum enMODBUS_RegisterBytes{
    MODBUS_REGISTER_1_BIT = 0,
    MODBUS_REGISTER_8_BITS = 1,
    MODBUS_REGISTER_16_BITS = 2,
    MODBUS_REGISTER_32_BITS = 4,
} registerBytes_t;

typedef enum{
    MODBUS_SET_RECEIVE = 0,
    MODBUS_SET_SEND,
} sendOrReceive_t;

// ENUMS //


// STRUCTS //

typedef struct {
    GPIO_TypeDef *sendReceivePort;
    uint16_t sendReceivePin;
    UART_HandleTypeDef *modbusUart;
    uint8_t deviceAddress;

    modbusStates_t modbusState;
    uint8_t payloadBuffer[MODBUS_BUFFER_SIZE];
    uint8_t payloadIndex;

    uint8_t requestId;
    modbusOpcodes_t opcode;
    uint16_t firstRegister;
    uint16_t qttRegisters;
    uint16_t qttBytes;
    uint32_t calculatedCRC;
} modbusHandler_t;

// STRUCTS //


// FUNCOES //

void MODBUS_Begin(modbusHandler_t *modbusHandler, GPIO_TypeDef *sendReceivePort, uint16_t sendReceivePin, UART_HandleTypeDef *huart, uint8_t deviceAddress);
void MODBUS_SetSendReceive(modbusHandler_t *modbusHandler, sendOrReceive_t sendOrReceive);
sendOrReceive_t MODBUS_GetSendReceive(modbusHandler_t *modbusHandler);
modbusError_t MODBUS_VerifyMessage(uint8_t expectedSecondaryAddress, uint8_t expectedOpcode, uint16_t expectedFirstAdress, uint16_t expectedNumberOfData, uint8_t *messageBuffer, uint32_t messageLength);
modbusError_t MODBUS_VerifyWithHandler(modbusHandler_t *modbusHandler, uint8_t *messageBuffer, uint32_t messageLength);
modbusError_t MODBUS_VerifyCrc(uint8_t *message, uint32_t length);
void MODBUS_ReadCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstCoilAddress, uint16_t numberOfCoils);
void MODBUS_ReadInputRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters);
void MODBUS_ReadSingleHoldingRegister(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress);
void MODBUS_ReadMultipleHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters, registerBytes_t sizeOfRegisterBytes);
void MODBUS_WriteSingleCoil(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t coilAddress, uint8_t valueToWrite);
void MODBUS_WriteMultipleCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstCoilAddress, uint16_t numberOfCoils, uint8_t *valuesToWrite);
void MODBUS_WriteSingleHoldingRegister(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress, uint32_t valueToWrite, registerBytes_t sizeOfRegisterBytes);
void MODBUS_WriteMultipleHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters, registerBytes_t sizeOfRegisterBytes, uint8_t *valuesToWrite);
void MODBUS_SendResponse(modbusHandler_t *modbusHandler, uint8_t *responseBuffer, uint16_t responseBufferLength);
void MODBUS_UpdateHandler(modbusHandler_t *modbusHandler, uint8_t *messageBuffer);
void MODBUS_SendError(modbusHandler_t *modbusHandler, modbusError_t error);

// FUNCOES //

#endif /* MODBUS_MODBUS_H_ */
