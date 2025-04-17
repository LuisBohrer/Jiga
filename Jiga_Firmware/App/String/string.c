/*
 * string.cpp
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#include "String/string.h"
#include <math.h>

void STRING_Init(string *self){
    self->length = 0;
    for(uint16_t i = 0; i < BUFFER_SIZE; i++)
        self->buffer[i] = '\0';
}

uint8_t* STRING_GetBuffer(string *self){
    return self->buffer;
}

uint16_t STRING_GetLength(const string *self){
    return self->length;
}

void STRING_AddChar(string *self, char character){
    self->buffer[self->length++] = character;
}

void STRING_AddInt(string *self, int32_t number){
    uint64_t i = pow(10, 10);
    if(number < 0){
        self->buffer[self->length++] = '-';
        number*=-1;
    }

    while(floor(number/i) == 0 && i > 1) i/=10;

    while(i >= 1){
        int32_t resto = number/i;
        self->buffer[self->length++] = resto%10 + '0';
        i/=10;
    }
}

void STRING_AddFloat(string *self, float number, uint32_t decimalSpaces, char separator){
    if(number < 0){
        self->buffer[self->length++] = '-';
        number*=-1;
    }
    STRING_AddInt(self, floor(number));
    STRING_AddChar(self, separator);
    uint32_t decimals = round(number*pow(10, decimalSpaces));
    uint32_t denominator = floor(pow(10, decimalSpaces));
    STRING_AddInt(self, decimals%denominator);
}

void STRING_AddCharString(string *self, const char* const inputCharString){
    for(uint32_t i = 0; inputCharString[i] != '\0'; i++){
        STRING_AddChar(self, inputCharString[i]);
    }
}

void STRING_AddString(string *self, const string *inputString){
    for(uint32_t i = 0; inputString->buffer[i] != '\0'; i++){
        STRING_AddChar(self, inputString->buffer[i]);
    }
}

void STRING_CopyString(const string *copyFrom, string *copyTo){
    STRING_Clear(copyTo);
    STRING_AddString(copyTo, copyFrom);
}

void STRING_Clear(string *self){
    STRING_Init(self);
}

uint8_t STRING_IsDigit(char inputChar){
    return inputChar >= '0' && inputChar <= '9';
}

uint8_t STRING_IsPrintable(char inputChar){
    return inputChar >= ' ' && inputChar <= '~';
}

void STRING_CharStringToString(const char* const inputCharString, string *outputString){
    STRING_Clear(outputString);
    STRING_AddCharString(outputString, inputCharString);
}

void STRING_StringToCharString(const string *inputString, char *outputCharString){
    for(uint32_t i = 0; i < inputString->length; i++){
        outputCharString[i] = inputString->buffer[i];
    }
}

int32_t STRING_StringToInt(const string *inputString){
    uint32_t i = 0;
    int32_t result = 0;
    int8_t signal = 1;
    while(i < inputString->length && !STRING_IsDigit(inputString->buffer[i])) i++;
    if(i >= 1){
        if(inputString->buffer[i - 1] == '-'){
            signal = -1;
        }
    }
    while(i < inputString->length && STRING_IsDigit(inputString->buffer[i])){
        result*=10;
        result += inputString->buffer[i] - '0';
        i++;
    }

    return result*signal;
}

float STRING_StringToFloat(const string *inputString, char separator){
    uint32_t i = 0;
    uint8_t separatorReached = 0;
    float decimalSpaces = 1.0;
    float result = 0.0;
    int8_t signal = 1;
    while(i < inputString->length && !STRING_IsDigit(inputString->buffer[i])) i++;
    if(i >= 1){
        if(inputString->buffer[i - 1] == '-'){
            signal = -1;
        }
    }
    while(i < inputString->length){
        uint8_t incomingChar = inputString->buffer[i];
        if(!separatorReached && incomingChar == separator){
            separatorReached = 1;
            i++;
            continue;
        }
        if(!separatorReached && STRING_IsDigit(incomingChar)){
            result*=10;
            result += incomingChar - '0';
        }
        else if(STRING_IsDigit(incomingChar)){
            decimalSpaces*=10;
            result += ((float)(incomingChar - '0'))/decimalSpaces;
        }
        else{
            break;
        }
        i++;
    }
    return result*signal;
}

uint8_t STRING_CompareStrings(const string *string1, const string *string2, uint16_t length){
    if(length > string1->length)
        length = string1->length;
    if(length > string2->length)
        length = string2->length;
    for(uint16_t i = 0; i < length; i++){
        if(string1->buffer[i] != string2->buffer[i]){
            return 0;
        }
    }
    return 1;
}

uint8_t STRING_CompareStringsRev(const string *string1, const string *string2, uint16_t length){
    if(length > string1->length)
        length = string1->length;
    if(length > string2->length)
        length = string2->length;
    for(uint16_t i = 0; i < length; i++){
        if(i <= string1->length || i <= string2->length)
            return 1;
        if(string1->buffer[string1->length - i - 1] != string2->buffer[string2->length - i - 1]){
            return 0;
        }
    }
    return 1;
}

uint8_t STRING_GetChar(const string *inputString, uint16_t index){
    if(index >= inputString->length)
        return 0;
    return inputString->buffer[index];
}
