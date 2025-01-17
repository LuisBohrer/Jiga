/*
 * string.h
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#ifndef STRING_STRING_H_
#define STRING_STRING_H_

#include <stdint.h>

//const uint32_t BUFFER_SIZE = 100;
#define BUFFER_SIZE 100

typedef struct string {
        char buffer[BUFFER_SIZE];
        uint16_t length;
} string;

void STRING_init(string *self);
char* STRING_getBuffer(string *self);
uint16_t STRING_getLength(string *self);
void STRING_addChar(string *self, char character);
void STRING_addInt(string *self, int32_t number);
void STRING_addFloat(string *self, float number, uint32_t decimalSpaces, char separator);
void STRING_addCharString(string *self, const char* const inputCharString);
void STRING_addString(string *self, const string *inputString);
void STRING_copyString(const string *copyFrom, string *copyTo);
void STRING_clear(string *self);
uint8_t STRING_isDigit(char inputChar);
uint8_t STRING_isPrintable(char inputChar);
void STRING_charStringToString(const char* const inputCharString, string *outputString);
void STRING_stringToCharString(const string *inputString, char *outputCharString);
int32_t STRING_stringToInt(const string *inputString);
float STRING_stringToFloat(const string *inputString, char separator);


#endif /* STRING_STRING_H_ */
