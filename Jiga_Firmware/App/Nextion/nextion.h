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
void NEXTION_SendStringMessage(string_t *message);
void NEXTION_SetComponentText(const string_t *component, const string_t *newText);
void NEXTION_SetComponentIntValue(const string_t *component, int32_t newValue);
void NEXTION_SetComponentFloatValue(const string_t *component, float newValue, uint32_t integerSpaces, uint32_t decimalSpaces);
void NEXTION_SetGlobalVariableValue(const string_t *variable, int32_t value);
displayResponses_t NEXTION_TreatMessage(string_t *message);

#endif /* NEXTION_NEXTION_H_ */
