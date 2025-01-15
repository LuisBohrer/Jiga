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
    STRING_init(&str);
    STRING_addInt(&str, 123);
}
