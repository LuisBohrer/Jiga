/*
 * string.h
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#ifndef STRING_STRING_H_
#define STRING_STRING_H_

#include <stdint.h>

#define BUFFER_SIZE 100

typedef struct string {
        uint8_t buffer[BUFFER_SIZE];
        uint16_t length;
} string;

void STRING_Init(string *self);
uint8_t* STRING_GetBuffer(string *self);
uint16_t STRING_GetLength(string *self);
void STRING_AddChar(string *self, char character);
void STRING_AddInt(string *self, int32_t number);
void STRING_AddFloat(string *self, float number, uint32_t decimalSpaces, char separator);
void STRING_AddCharString(string *self, const char* const inputCharString);
void STRING_AddString(string *self, const string *inputString);
void STRING_CopyString(const string *copyFrom, string *copyTo);
void STRING_Clear(string *self);
uint8_t STRING_IsDigit(char inputChar);
uint8_t STRING_IsPrintable(char inputChar);
void STRING_CharStringToString(const char* const inputCharString, string *outputString);
void STRING_StringToCharString(const string *inputString, char *outputCharString);
int32_t STRING_StringToInt(const string *inputString);
float STRING_StringToFloat(const string *inputString, char separator);
uint8_t STRING_CompareStrings(const string *string1, const string *string2, uint16_t length);
uint8_t STRING_CompareStringsRev(const string *string1, const string *string2, uint16_t length);
uint8_t STRING_GetChar(const string *inputString, uint16_t index);

#endif /* STRING_STRING_H_ */
