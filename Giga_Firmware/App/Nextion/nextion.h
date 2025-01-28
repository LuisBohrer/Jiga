/*
 * nextion.h
 *
 *  Created on: Jan 16, 2025
 *      Author: luisv
 */

#ifndef NEXTION_NEXTION_H_
#define NEXTION_NEXTION_H_

#include "String/string.h"
#include "usart.h"

#define DISPLAY_UART_DEFAULT_TIMEOUT 100

typedef enum{
    NO_MESSAGE = 0,
    INCOMPLETE_MESSAGE,
    ERROR_INVALID_VARIABLE,
    ERROR_BUFFER_OVERFLOW,
    VALID_MESSAGE
} displayResponses_t;

void NEXTION_Begin(UART_HandleTypeDef *displayUartAddress);
void NEXTION_SendCharMessage(const char* const message);
void NEXTION_SendStringMessage(string *message);
void NEXTION_SetComponentText(const string *component, const string *newText);
void NEXTION_SetComponentIntValue(const string *component, int32_t newValue);
void NEXTION_SetComponentFloatValue(const string *component, float newValue, uint32_t integerSpaces, uint32_t decimalSpaces);
void NEXTION_SetGlobalVariableValue(const string *variable, int32_t value);
displayResponses_t NEXTION_TreatMessage(string *message);

#endif /* NEXTION_NEXTION_H_ */
