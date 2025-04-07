/*
 * leds.c
 *
 *  Created on: Aug 9, 2024
 *      Author: luisv
 */

#include "Leds/leds.h"
#include "main.h"
#include "tim.h"

const uint16_t NUMBER_OF_LEDS = 2;

uint16_t led1BlinkPeriod = 0;
uint16_t led2BlinkPeriod = 0;
uint16_t led3BlinkPeriod = 0;
uint16_t led4BlinkPeriod = 0;

void vLEDS_SetLedState(uint8_t ui8LedNumber, GPIO_PinState estado){
    switch(ui8LedNumber){
        case 1:
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, estado);
            break;
        case 2:
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, estado);
            break;
        default:
            break;
    }
}

GPIO_PinState ui8LEDS_GetLedState(uint8_t ui8LedNumber){
    switch(ui8LedNumber){
        case 1:
            return HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
        case 2:
            return HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin);
        default:
            return 0;
    }
}

void vLEDS_ToggleLed(uint8_t ui8LedNumber){
    uint8_t vLEDS_ToggleLed_estado = !ui8LEDS_GetLedState(ui8LedNumber);
    switch(ui8LedNumber){
        case 1:
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, vLEDS_ToggleLed_estado);
            break;
        case 2:
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, vLEDS_ToggleLed_estado);
            break;
        default:
            break;
    }
}

void vLEDS_AcendeTodos(){
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
}

void vLEDS_ApagaTodos(){
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

void vLEDS_StartBlinkLed(uint8_t ui8LedNumber, uint16_t blinkPeriod_ms){
    if(ui8LedNumber > NUMBER_OF_LEDS)
        return;

    switch(ui8LedNumber){
        case 1:
            led1BlinkPeriod = blinkPeriod_ms;
            break;
        case 2:
            led2BlinkPeriod = blinkPeriod_ms;
            break;
        default:
            break;
    }
}

void vLEDS_StopBlinkLed(uint8_t ui8LedNumber){
    if(ui8LedNumber > NUMBER_OF_LEDS)
        return;

    switch(ui8LedNumber){
        case 1:
            led1BlinkPeriod = 0;
            vLEDS_SetLedState(1, GPIO_PIN_RESET);
            break;
        case 2:
            led2BlinkPeriod = 0;
            vLEDS_SetLedState(2, GPIO_PIN_RESET);
            break;
        default:
            break;
    }
}

void vLEDS_LedsTimerCallback(){
    static uint16_t led1Counter = 0;
    static uint16_t led2Counter = 0;

    if(led1BlinkPeriod){
        if(led1Counter < led1BlinkPeriod){
            led1Counter++;
        }
        else{
            vLEDS_ToggleLed(1);
            led1Counter = 0;
        }
    }

    if(led2BlinkPeriod){
        if(led2Counter < led2BlinkPeriod){
            led2Counter++;
        }
        else{
            vLEDS_ToggleLed(2);
            led2Counter = 0;
        }
    }
}

