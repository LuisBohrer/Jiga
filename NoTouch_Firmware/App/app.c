/*
 * app.c
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#include "app.h"
#include "String/string.h"

void APP_init(){
    string str;
    STRING_addCharString(&str, "abc/0");
    STRING_addInt(&str, 123);
    STRING_addCharString(&str, ",456");
    STRING_addChar(&str, 'T');
    float num = STRING_stringToFloat(&str, ',');
}
