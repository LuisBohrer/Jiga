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

void NEXTION_init(){
    string msg;
    STRING_copyString(&currentTxtBx[0], &msg);
}

void NEXTION_sendCharMessage(const char* const message){
    string messageString;
    STRING_charStringToString(message, &messageString);
    STRING_addString(&messageString, &endPacket);
    // hal transmit com string_getbuffer
}

void NEXTION_sendStringMessage(string *message){
    STRING_addString(message, &endPacket);
    HAL_UART_Transmit(&hlpuart1,
            STRING_getBuffer(message),
            STRING_getLength(message),
            DISPLAY_UART_DEFAULT_TIMEOUT);
}

void NEXTION_setComponentText(const string *component, const string *newText){
    string nextionMessage;
    STRING_copyString(component, &nextionMessage);
    STRING_addString(&nextionMessage, &textSufix);
    STRING_addCharString(&nextionMessage, "=\"");
    STRING_addString(&nextionMessage, newText);
    STRING_addChar(&nextionMessage, '"');
    NEXTION_sendStringMessage(&nextionMessage);
}

void NEXTION_setComponentIntValue(const string *component, int32_t newValue){
    string nextionMessage;
    STRING_copyString(component, &nextionMessage);
    STRING_addString(&nextionMessage, &valueSufix);
    STRING_addChar(&nextionMessage, '=');
    STRING_addInt(&nextionMessage, newValue);
    NEXTION_sendStringMessage(&nextionMessage);
}

void NEXTION_setComponentFloatValue(const string *component, float newValue, uint32_t decimalSpaces){
    string nextionMessage;
    STRING_copyString(component, &nextionMessage);
    STRING_addString(&nextionMessage, &valueSufix);
    STRING_addChar(&nextionMessage, '=');
    STRING_addInt(&nextionMessage, newValue*pow(10, decimalSpaces));
    NEXTION_sendStringMessage(&nextionMessage);
}

void NEXTION_setGlobalVariableValue(const string *variable, int32_t value){
    string nextionMessage;
    STRING_copyString(variable, &nextionMessage);
    STRING_addChar(&nextionMessage, '=');
    STRING_addInt(&nextionMessage, value);
    NEXTION_sendStringMessage(&nextionMessage);
}

void NEXTION_updateReads(uint8_t readingCurrent, uint16_t newReads[10]){

}
