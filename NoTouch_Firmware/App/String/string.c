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

char* STRING_getBuffer(string *self){
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

    while(floor(number/i) == 0 && i > 1) i/=10;

    while(i >= 1){
        uint32_t resto = number/i;
        self->buffer[self->length++] = resto%10 + '0';
        i/=10;
    }
}

void STRING_addFloat(string *self, float number, uint32_t decimalSpaces, char separator){
    STRING_addInt(self, floor(number));
    STRING_addChar(self, separator);
    uint32_t decimals = round(number*pow(10, decimalSpaces));
    uint32_t denominator = floor(pow(10, decimalSpaces));
    STRING_addInt(self, decimals%denominator);
}

void STRING_clear(string *self){
    STRING_init(self);
}

uint8_t STRING_isDigit(char inputChar){
    return inputChar >= '0' && inputChar <= '9';
}

void STRING_addCharString(string *self, const char* const inputCharString){
    for(uint32_t i = 0; inputCharString[i] != '\0'; i++){
        STRING_addChar(self, inputCharString[i]);
    }
}

void STRING_charStringToString(const char* const inputCharString, string *outputString){
    STRING_clear(outputString);
    STRING_addCharString(outputString, inputCharString);
}

void STRING_stringToCharString(string *inputString, char *outputCharString){
    for(uint32_t i = 0; i < inputString->length; i++){
        outputCharString[i] = inputString->buffer[i];
    }
}

uint32_t STRING_stringToInt(string *inputString){
    uint32_t i = 0;
    uint32_t result = 0;
    while(i < inputString->length && !STRING_isDigit(inputString->buffer[i])) i++;
    while(i < inputString->length && STRING_isDigit(inputString->buffer[i])){
        result*=10;
        result += inputString->buffer[i] - '0';
        i++;
    }

    return result;
}

float STRING_stringToFloat(string *inputString, char separator){
    uint32_t i = 0;
    uint8_t separatorReached = 0;
    float decimalSpaces = 1;
    float result = 0.0;
    while(i < inputString->length && !STRING_isDigit(inputString->buffer[i])) i++;
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
    return result;
}
