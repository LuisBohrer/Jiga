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

void STRING_addFloat(string *self, float number, uint32_t decimalSpaces){

}

void STRING_clear(string *self){

}
