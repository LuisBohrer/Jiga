/*
 * nextion.h
 *
 *  Created on: Jan 16, 2025
 *      Author: luisv
 */

#ifndef NEXTION_NEXTION_H_
#define NEXTION_NEXTION_H_

#include "String/string.h"

void NEXTION_init();
void NEXTION_sendCharMessage(const char* const message);
void NEXTION_sendStringMessage(string *message);
void NEXTION_setComponentText(const string *component, const string *newText);
void NEXTION_setComponentIntValue(const string *component, int32_t newValue);
void NEXTION_setComponentFloatValue(const string *component, float newValue, uint32_t decimalSpaces);
void NEXTION_setGlobalVariableValue(const string *variable, int32_t value);

#endif /* NEXTION_NEXTION_H_ */
