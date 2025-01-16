/*
 * app.c
 *
 *  Created on: Jan 15, 2025
 *      Author: luisv
 */

#include "app.h"
#include "String/string.h"

void APP_init(){
    string str1, str2;
    STRING_init(&str1);
    STRING_init(&str2);
    STRING_addCharString(&str1, "abc\0");
    STRING_addInt(&str1, -123);
    STRING_addCharString(&str1, ",456");
    STRING_addChar(&str2, 'T');
    STRING_addString(&str2, &str1);
    STRING_copyString(&str1, &str2);
    float num = STRING_stringToInt(&str1);
    num = STRING_stringToFloat(&str1, ',');
}
