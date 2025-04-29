/*
 * comm.h
 *
 *  Created on: Jan 23, 2025
 *      Author: luisv
 */

#ifndef COMM_COMM_H_
#define COMM_COMM_H_

#include "usart.h"
#include "String/string.h"

typedef enum{
    INCOMPLETE_REQUEST,
    INVALID_REQUEST,
    CALIBRATE_VOLTAGE_MIN,
    CALIBRATE_VOLTAGE_MAX,
    CALIBRATE_CURRENT_MIN,
    CALIBRATE_CURRENT_MAX,
    SEND_VOLTAGE_READS = 22,
    SEND_CURRENT_READS,
    SEND_ALL_READS,
    SET_MODBUS_CONFIG,
    LOGS,
} debugRequest_t;

typedef enum{
    NACK = 1,
    ACK_VOLTAGE_READS = 22,
    ACK_CURRENT_READS,
    ACK_ALL_READS,
    ACK_LOGS,
    ACK_MODBUS_CONFIG,
    ACK_CHANGE_SCALE,
} debugAckSignals_t;

void COMM_Begin(UART_HandleTypeDef *huart);
void COMM_SendStartPacket(void);
void COMM_SendEndPacket(void);
void COMM_SendAck(debugAckSignals_t ack);
void COMM_SendValues8Bits(uint8_t *values, uint16_t length);
void COMM_SendValues16Bits(uint16_t *values, uint16_t length);
void COMM_SendValues32Bits(uint32_t *values, uint16_t length);
void COMM_SendString(string *message);
void COMM_SendChar(uint8_t *buffer, uint16_t length);
debugRequest_t COMM_TreatResponse(string *message);


#endif /* COMM_COMM_H_ */
