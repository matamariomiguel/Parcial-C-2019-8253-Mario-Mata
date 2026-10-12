#ifndef LED_APP_H
#define LED_APP_H

#include <stdint.h>

void Led_Init(void);
void Led_SetSolid(void);                 /* encendido fijo */
void Led_SetBlink(uint32_t periodMs);    /* alterna cada periodMs */
void Led_Service(uint32_t nowMs);        /* llamar en cada vuelta de main */

#endif
