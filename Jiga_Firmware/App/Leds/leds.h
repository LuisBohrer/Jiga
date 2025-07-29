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

void LEDS_SetLedState(uint8_t ledNumber, GPIO_PinState estado);
GPIO_PinState LEDS_GetLedState(uint8_t ledNumber);
void LEDS_ToggleLed(uint8_t ledNumber);
void LEDS_AcendeTodos();
void LEDS_ApagaTodos();
void LEDS_StartBlinkLed(uint8_t ledNumber, uint16_t blinkPeriod_ms);
void LEDS_StopBlinkLed(uint8_t ledNumber);
void LEDS_LedsTimerCallback(void);

#endif /* LEDS_LEDS_H_ */
