/*
 * string.cpp
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#include "string.h"
#include <math.h>

void STRING_init(string *self){
    self->length = 0;
    for(uint16_t i = 0; i < BUFFER_SIZE; i++)
        self->buffer[i] = '\0';
}

uint8_t* STRING_getBuffer(string *self){
    return self->buffer;
}

uint16_t STRING_getLength(string *self){
    return self->length;
}

void STRING_addChar(string *self, char character){
    self->buffer[self->length++] = character;
}

void STRING_addInt(string *self, int32_t number){
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

void STRING_addFloat(string *self, float number, uint32_t decimalSpaces, char separator){
    if(number < 0){
        self->buffer[self->length++] = '-';
        number*=-1;
    }
    STRING_addInt(self, floor(number));
    STRING_addChar(self, separator);
    uint32_t decimals = round(number*pow(10, decimalSpaces));
    uint32_t denominator = floor(pow(10, decimalSpaces));
    STRING_addInt(self, decimals%denominator);
}

void STRING_addCharString(string *self, const char* const inputCharString){
    for(uint32_t i = 0; inputCharString[i] != '\0'; i++){
        STRING_addChar(self, inputCharString[i]);
    }
}

void STRING_addString(string *self, const string *inputString){
    for(uint32_t i = 0; inputString->buffer[i] != '\0'; i++){
        STRING_addChar(self, inputString->buffer[i]);
    }
}

void STRING_copyString(const string *copyFrom, string *copyTo){
    STRING_clear(copyTo);
    STRING_addString(copyTo, copyFrom);
}

void STRING_clear(string *self){
    STRING_init(self);
}

uint8_t STRING_isDigit(char inputChar){
    return inputChar >= '0' && inputChar <= '9';
}

uint8_t STRING_isPrintable(char inputChar){
    return inputChar >= ' ' && inputChar <= '~';
}

void STRING_charStringToString(const char* const inputCharString, string *outputString){
    STRING_clear(outputString);
    STRING_addCharString(outputString, inputCharString);
}

void STRING_stringToCharString(const string *inputString, char *outputCharString){
    for(uint32_t i = 0; i < inputString->length; i++){
        outputCharString[i] = inputString->buffer[i];
    }
}

int32_t STRING_stringToInt(const string *inputString){
    uint32_t i = 0;
    int32_t result = 0;
    int8_t signal = 1;
    while(i < inputString->length && !STRING_isDigit(inputString->buffer[i])) i++;
    if(i >= 1){
        if(inputString->buffer[i - 1] == '-'){
            signal = -1;
        }
    }
    while(i < inputString->length && STRING_isDigit(inputString->buffer[i])){
        result*=10;
        result += inputString->buffer[i] - '0';
        i++;
    }

    return result*signal;
}

float STRING_stringToFloat(const string *inputString, char separator){
    uint32_t i = 0;
    uint8_t separatorReached = 0;
    float decimalSpaces = 1.0;
    float result = 0.0;
    int8_t signal = 1;
    while(i < inputString->length && !STRING_isDigit(inputString->buffer[i])) i++;
    if(i >= 1){
        if(inputString->buffer[i - 1] == '-'){
            signal = -1;
        }
    }
    while(i < inputString->length){
        uint8_t incomingChar = inputString->buffer[i];
        if(incomingChar == separator){
            separatorReached = 1;
            i++;
            continue;
        }
        if(!separatorReached && STRING_isDigit(incomingChar)){
            result*=10;
            result += incomingChar - '0';
        }
        else if(STRING_isDigit(incomingChar)){
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

uint8_t STRING_compareStrings(const string *string1, const string *string2, uint16_t length){
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

uint8_t STRING_compareStringsRev(const string *string1, const string *string2, uint16_t length){
    for(uint16_t i = 0; i < length; i++){
        if(i <= string1->length || i <= string2->length)
            return 1;
        if(string1->buffer[string1->length - i - 1] != string2->buffer[string2->length - i - 1]){
            return 0;
        }
    }
    return 1;
}

uint8_t STRING_getChar(const string *inputString, uint16_t index){
    if(index >= inputString->length)
        return 0;
    return inputString->buffer[index];
}
