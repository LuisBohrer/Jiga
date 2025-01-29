/*
 * modbus.h
 *
 *  Created on: Aug 23, 2024
 *      Author: luisv
 */

#ifndef MODBUS_MODBUS_H_
#define MODBUS_MODBUS_H_

// INCLUDES //

#include "RingBuffer/ringBuffer.h"
#include "gpio.h"

// INCLUDES //


// DEFINES / CONSTANTES //

#define MODBUS_BUFFER_SIZE 500
#define MODBUS_FIRST_ADDRESS 0xC8
#define MODBUS_MAX_REGISTER 10

// DEFINES / CONSTANTES //


// ENUMS //

typedef enum enMODBUS_ModbusStates{
    MODBUS_IDLE = 0,
    MODBUS_SENDING,
    MODBUS_RECEIVING,
    MODBUS_PROCESS_MESSAGE,
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
    MODBUS_INVALID_OPCODE = 0,
    MODBUS_RESPONSE_ERROR,
    MODBUS_INVALID_REGISTER_ADDRESS,
    MODBUS_TIMEOUT,
    MODBUS_INCORRECT_ID,
    MODBUS_INCORRECT_OPCODE,
    MODBUS_INCORRECT_FIRST_REGISTER,
    MODBUS_INCORRECT_QTT_REGISTERS,
    MODBUS_INCORRECT_CRC,
    MODBUS_INVALID_MESSAGE,
} modbusError_t;

typedef enum enMODBUS_RegisterBytes{
    MODBUS_REGISTER_8_BITS = 1,
    MODBUS_REGISTER_16_BITS = 2,
    MODBUS_REGISTER_32_BITS = 4,
} registerBytes_t;

// ENUMS //


// STRUCTS //

typedef struct {
    GPIO_TypeDef *sendReceivePort;
    uint16_t sendReceivePin;

    modbusStates_t ModbusState;
    ringBuffer_t SendCommandRingBuffer;
    uint8_t PayloadBuffer[MODBUS_BUFFER_SIZE];
    uint8_t PayloadIndex;
    uint8_t DeviceAddress;
    uint8_t RequestId;
    modbusOpcodes_t Opcode;
    uint16_t FirstRegister;
    uint16_t QttRegisters;
    uint8_t QttBytes;
    uint8_t DataSize;
    uint8_t DataBuffer[MODBUS_BUFFER_SIZE];
    uint8_t DataIndex;
    uint8_t DataComplete;
    uint8_t registerSize;
    uint32_t ExpectedCRC;
    uint32_t CalculatedCRC;
    uint32_t RegisterReads[MODBUS_MAX_REGISTER];
    uint32_t timeout_100ms;
} modbusHandler_t;

// STRUCTS //


// FUNCOES //

void MODBUS_Init(modbusHandler_t *modbusHandler, GPIO_TypeDef *sendReceivePort, uint16_t sendReceivePin, UART_HandleTypeDef *huart);
void vMODBUS_Poll(modbusHandler_t *modbusHandler);
//void vMODBUS_EnableModbus(void);
//void vMODBUS_DisableModbus(void);
void vMODBUS_Read(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint8_t command, uint16_t offset, uint16_t numberOfRegisters, registerBytes_t sizeOfRegistersBytes);
void vMODBUS_Write(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint8_t command, uint16_t offset, uint16_t numberOfRegisters, uint8_t numberOfParameterBytes, uint16_t* parameters);
void MODBUS_ReadCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t coilAddress, uint16_t numberOfCoils);
void MODBUS_ReadInputRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress);
void MODBUS_ReadSingleHoldingRegister(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress);
void MODBUS_ReadMultipleHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters, registerBytes_t sizeOfRegisterBytes);
void MODBUS_WriteSingleCoil(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t coilAddress, uint8_t valueToWrite);
void MODBUS_WriteMultipleCoils(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstCoilAddress, uint16_t numberOfCoils, uint8_t *valuesToWrite);
void MODBUS_WriteSingleHoldingRegister(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t registerAddress, uint32_t valueToWrite, registerBytes_t sizeOfRegisterBytes);
void MODBUS_WriteMultipleHoldingRegisters(modbusHandler_t *modbusHandler, uint8_t secondaryAddress, uint16_t firstRegisterAddress, uint16_t numberOfRegisters, registerBytes_t sizeOfRegisterBytes, uint8_t* valuesToWrite);

// FUNCOES //

#endif /* MODBUS_MODBUS_H_ */
