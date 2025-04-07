/*
 * leds.h
 *
 *  Created on: Aug 9, 2024
 *      Author: luisv
 */

#ifndef LEDS_LEDS_H_
#define LEDS_LEDS_H_

#include <stdint.h>
#include "main.h"

void vLEDS_SetLedState(uint8_t ui8LedNumber, GPIO_PinState estado);
GPIO_PinState ui8LEDS_GetLedState(uint8_t ui8LedNumber);
void vLEDS_ToggleLed(uint8_t ui8LedNumber);
void vLEDS_AcendeTodos();
void vLEDS_ApagaTodos();
void vLEDS_StartBlinkLed(uint8_t ui8LedNumber, uint16_t blinkPeriod_ms);
void vLEDS_StopBlinkLed(uint8_t ui8LedNumber);
void vLEDS_LedsTimerCallback(void);

#endif /* LEDS_LEDS_H_ */
