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

const string textSufix = {".txt", 4};
const string valueSufix = {".val", 4};
const string endPacket = {{0xff, 0xff, 0xff}, 3};
UART_HandleTypeDef* displayUart;

void NEXTION_Begin(UART_HandleTypeDef *displayUartAddress){
    displayUart = displayUartAddress;
}

void NEXTION_SendCharMessage(const char* const message){
    string messageString;
    STRING_CharStringToString(message, &messageString);
    NEXTION_SendStringMessage(&messageString);
}

void NEXTION_SendStringMessage(string *message){
    STRING_AddString(message, &endPacket);
    HAL_UART_Transmit(displayUart,
            STRING_GetBuffer(message),
            STRING_GetLength(message),
            DISPLAY_UART_DEFAULT_TIMEOUT);
}

void NEXTION_SetComponentText(const string *component, const string *newText){
    string nextionMessage;
    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &textSufix);
    STRING_AddCharString(&nextionMessage, "=\"");
    STRING_AddString(&nextionMessage, newText);
    STRING_AddChar(&nextionMessage, '"');
    NEXTION_SendStringMessage(&nextionMessage);
}

void NEXTION_SetComponentIntValue(const string *component, int32_t newValue){
    string nextionMessage;
    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &valueSufix);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, newValue);
    NEXTION_SendStringMessage(&nextionMessage);
}

void NEXTION_SetComponentFloatValue(const string *component, float newValue, uint32_t decimalSpaces){
    string nextionMessage;
    STRING_CopyString(component, &nextionMessage);
    STRING_AddString(&nextionMessage, &valueSufix);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, newValue*pow(10, decimalSpaces));
    NEXTION_SendStringMessage(&nextionMessage);
}

void NEXTION_SetGlobalVariableValue(const string *variable, int32_t value){
    string nextionMessage;
    STRING_CopyString(variable, &nextionMessage);
    STRING_AddChar(&nextionMessage, '=');
    STRING_AddInt(&nextionMessage, value);
    NEXTION_SendStringMessage(&nextionMessage);
}


displayResponses_t NEXTION_TreatMessage(ringBuffer_t *buffer, string *message){
    while(!RB_IsEmpty(buffer)){
        STRING_AddChar(message, RB_GetByte(buffer));
    }

    if(message->length <= 0){
        return NO_MESSAGE;
    }
    if(!STRING_CompareStringsRev(message, &endPacket, 3)){
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
