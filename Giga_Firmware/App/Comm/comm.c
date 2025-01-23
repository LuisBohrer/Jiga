/*
 * comm.c
 *
 *  Created on: Jan 23, 2025
 *      Author: luisv
 */

#include "comm.h"
#include "usart.h"
#include "String/string.h"

UART_HandleTypeDef *debugUart;
const uint32_t DEFAULT_UART_TIMEOUT = 200;

#define SIZE_OF_DEBUG_START_PACKET 2
const uint8_t debugStartPacket[SIZE_OF_DEBUG_START_PACKET] = {0x23, 0x23};
#define SIZE_OF_DEBUG_END_PACKET 1
const uint8_t debugEndPacket[SIZE_OF_DEBUG_END_PACKET] = {0x40};

void COMM_Begin(UART_HandleTypeDef *huart){
    debugUart = huart;
}

void COMM_SendValues8Bits(uint8_t *values, uint16_t length){
    HAL_UART_Transmit(debugUart, debugStartPacket, SIZE_OF_DEBUG_START_PACKET, DEFAULT_UART_TIMEOUT);
    HAL_UART_Transmit(debugUart, values, length, DEFAULT_UART_TIMEOUT);
    HAL_UART_Transmit(debugUart, debugEndPacket, SIZE_OF_DEBUG_END_PACKET, DEFAULT_UART_TIMEOUT);
}

void COMM_SendValues16Bits(uint16_t *values, uint16_t length){
    COMM_SendValues8Bits((uint8_t*) values, 2*length);
}

void COMM_SendValues32Bits(uint32_t *values, uint16_t length){
    COMM_SendValues8Bits((uint8_t*) values, 4*length);
}

void COMM_SendString(string *message){
    HAL_UART_Transmit(debugUart, debugStartPacket, SIZE_OF_DEBUG_START_PACKET, DEFAULT_UART_TIMEOUT);
    HAL_UART_Transmit(debugUart, STRING_GetBuffer(message), STRING_GetLength(message), DEFAULT_UART_TIMEOUT);
    HAL_UART_Transmit(debugUart, debugEndPacket, SIZE_OF_DEBUG_END_PACKET, DEFAULT_UART_TIMEOUT);
}

void COMM_SendChar(uint8_t *buffer, uint16_t length){
    HAL_UART_Transmit(debugUart, debugStartPacket, SIZE_OF_DEBUG_START_PACKET, DEFAULT_UART_TIMEOUT);
    HAL_UART_Transmit(debugUart, buffer, length, DEFAULT_UART_TIMEOUT);
    HAL_UART_Transmit(debugUart, debugEndPacket, SIZE_OF_DEBUG_END_PACKET, DEFAULT_UART_TIMEOUT);
}

debugRequest_t COMM_TreatResponse(string *message){
    return INVALID_REQUEST;
}
