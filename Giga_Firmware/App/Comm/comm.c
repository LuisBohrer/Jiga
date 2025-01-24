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
//const uint8_t debugStartPacket[SIZE_OF_DEBUG_START_PACKET] = {0x23, 0x23};
string debugStartPacket = {{0x23, 0x23}, 2};
#define SIZE_OF_DEBUG_END_PACKET 1
//const uint8_t debugEndPacket[SIZE_OF_DEBUG_END_PACKET] = {0x40};
string debugEndPacket = {{0x40}, 1};

void COMM_Begin(UART_HandleTypeDef *huart){
    debugUart = huart;
}

void COMM_SendStartPacked(){
    HAL_UART_Transmit(debugUart, STRING_GetBuffer(&debugStartPacket), STRING_GetLength(&debugStartPacket), DEFAULT_UART_TIMEOUT);
}

void COMM_SendEndPacked(){
    HAL_UART_Transmit(debugUart, STRING_GetBuffer(&debugEndPacket), STRING_GetLength(&debugEndPacket), DEFAULT_UART_TIMEOUT);
}

void COMM_SendAck(debugAckSignals_t ack){
    uint8_t message = ack;
    HAL_UART_Transmit(debugUart, &message, 1, DEFAULT_UART_TIMEOUT);
}

void COMM_SendValues8Bits(uint8_t *values, uint16_t length){
    HAL_UART_Transmit(debugUart, values, length, DEFAULT_UART_TIMEOUT);
}

void COMM_SendValues16Bits(uint16_t *values, uint16_t length){
    COMM_SendValues8Bits((uint8_t*) values, 2*length);
}

void COMM_SendValues32Bits(uint32_t *values, uint16_t length){
    COMM_SendValues8Bits((uint8_t*) values, 4*length);
}

void COMM_SendString(string *message){
    HAL_UART_Transmit(debugUart, STRING_GetBuffer(message), STRING_GetLength(message), DEFAULT_UART_TIMEOUT);
}

void COMM_SendChar(uint8_t *buffer, uint16_t length){
    HAL_UART_Transmit(debugUart, buffer, length, DEFAULT_UART_TIMEOUT);
}

debugRequest_t COMM_TreatResponse(string *message){
    if(STRING_GetLength(message) <= 3)
        return INCOMPLETE_REQUEST;
    if(!STRING_CompareStrings(message, &debugStartPacket, STRING_GetLength(&debugStartPacket)))
        return INVALID_REQUEST;
    if(!STRING_CompareStringsRev(message, &debugEndPacket, STRING_GetLength(&debugEndPacket)))
        return INVALID_REQUEST;
    if(STRING_GetChar(message, 2) == INCOMPLETE_REQUEST)
        return INVALID_REQUEST;

    return (debugRequest_t) STRING_GetChar(message, 2);
}
