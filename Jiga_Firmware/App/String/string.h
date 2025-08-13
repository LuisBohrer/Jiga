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

typedef struct string_s {
        uint8_t buffer[BUFFER_SIZE];
        uint16_t length;
} string_t;

void STRING_Init(string_t *self);
uint8_t* STRING_GetBuffer(string_t *self);
uint16_t STRING_GetLength(const string_t *self);
void STRING_AddChar(string_t *self, char character);
void STRING_AddInt(string_t *self, int32_t number);
void STRING_AddFloat(string_t *self, float number, uint32_t decimalSpaces, char separator);
void STRING_AddCharString(string_t *self, const char* const inputCharString);
void STRING_AddString(string_t *self, const string_t *inputString);
void STRING_CopyString(const string_t *copyFrom, string_t *copyTo);
void STRING_Clear(string_t *self);
uint8_t STRING_IsDigit(char inputChar);
uint8_t STRING_IsPrintable(char inputChar);
void STRING_CharStringToString(const char* const inputCharString, string_t *outputString);
void STRING_StringToCharString(const string_t *inputString, char *outputCharString);
int32_t STRING_StringToInt(const string_t *inputString);
float STRING_StringToFloat(const string_t *inputString, char separator);
uint8_t STRING_CompareStrings(const string_t *string1, const string_t *string2, uint16_t length);
uint8_t STRING_CompareStringsRev(const string_t *string1, const string_t *string2, uint16_t length);
uint8_t STRING_GetChar(const string_t *inputString, uint16_t index);

#endif /* STRING_STRING_H_ */
