/*
 * nextion.c
 *
 *  Created on: Jan 16, 2025
 *      Author: luisv
 */

#include "Nextion/nextion.h"
#include "String/string.h"
#include "Nextion/nextionComponents.h"
#include <math.h>
#include "usart.h"

const string_t textSufix = {".txt", 4};
const string_t valueSufix = {".val", 4};
const string_t numberOfIntSufix = {".vvs0", 5};
const string_t numberOfDecSufix = {".vvs1", 5};
const string_t nextionEndPacket = {{0xff, 0xff, 0xff}, 3};
UART_HandleTypeDef* displayUart;

void NEXTION_Begin(UART_HandleTypeDef *displayUartAddress){
    displayUart = displayUartAddress;
}

void NEXTION_SendCharMessage(const char* const message){
    string_t messageString;
    STRING_CharStringToString(message, &messageString);
    NEXTION_SendStringMessage(&messageString);
}

void NEXTION_SendStringMessage(string_t *message){
    STRING_AddString(message, &nextionEndPacket);
    HAL_UART_Transmit(displayUart,
            STRING_GetBuffer(message),
            STRING_GetLength(message),
            DISPLAY_UART_DEFAULT_TIMEOUT);
}

void NEXTION_SetComponentText(const string_t *component, const string_t *newText){
    string_t nextionMessage;
    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &textSufix);
    STRING_AddCharString(&nextionMessage, "=\"");
    STRING_AddString(&nextionMessage, newText);
    STRING_AddChar(&nextionMessage, '"');
    NEXTION_SendStringMessage(&nextionMessage);
}

void NEXTION_SetComponentIntValue(const string_t *component, int32_t newValue){
    string_t nextionMessage;
    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &valueSufix);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, newValue);
    NEXTION_SendStringMessage(&nextionMessage);
}

void NEXTION_SetComponentFloatValue(const string_t *component, float newValue, uint32_t integerSpaces, uint32_t decimalSpaces){
    string_t nextionMessage;
    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &numberOfIntSufix);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, integerSpaces);
    NEXTION_SendStringMessage(&nextionMessage);

    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &numberOfDecSufix);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, decimalSpaces);
    NEXTION_SendStringMessage(&nextionMessage);

    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &valueSufix);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, newValue*pow(10, decimalSpaces));
    NEXTION_SendStringMessage(&nextionMessage);
}

void NEXTION_SetGlobalVariableValue(const string_t *variable, int32_t value){
    string_t nextionMessage;
    STRING_CopyString(variable, &nextionMessage);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, value);
    NEXTION_SendStringMessage(&nextionMessage);
}

displayResponses_t NEXTION_TreatMessage(string_t *message){
    if(message->length <= 0){
        return NO_MESSAGE;
    }
    if(!STRING_CompareStringsRev(message, &nextionEndPacket, 3)){
        return INCOMPLETE_MESSAGE;
    }
    if(STRING_GetChar(message, 0) == 0x1A){
        return ERROR_INVALID_VARIABLE;
    }
    if(STRING_GetChar(message, 0) == 0x24){
        return ERROR_BUFFER_OVERFLOW;
    }
    return VALID_MESSAGE;
}
