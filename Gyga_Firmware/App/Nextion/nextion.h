/*
 * nextion.h
 *
 *  Created on: Jan 16, 2025
 *      Author: luisv
 */

#ifndef NEXTION_NEXTION_H_
#define NEXTION_NEXTION_H_

#include "String/string.h"
#include "RingBuffer/ringBuffer.h"

#define DISPLAY_UART_DEFAULT_TIMEOUT 100

typedef enum{
    NO_MESSAGE = 0,
    INCOMPLETE_MESSAGE,
    ERROR_INVALID_VARIABLE,
    ERROR_BUFFER_OVERFLOW,
    VALID_MESSAGE
} displayResponses_t;

void NEXTION_sendCharMessage(const char* const message);
void NEXTION_sendStringMessage(string *message);
void NEXTION_setComponentText(const string *component, const string *newText);
void NEXTION_setComponentIntValue(const string *component, int32_t newValue);
void NEXTION_setComponentFloatValue(const string *component, float newValue, uint32_t decimalSpaces);
void NEXTION_setGlobalVariableValue(const string *variable, int32_t value);
displayResponses_t NEXTION_treatMessage(ringBuffer_t *buffer, string *message);

#endif /* NEXTION_NEXTION_H_ */
