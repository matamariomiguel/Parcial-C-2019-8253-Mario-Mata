#ifndef PWM_APP_H
#define PWM_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "fsl_common.h"

status_t Pwm_Init(uint32_t *clockHz);
status_t Pwm_SetDuty(uint8_t dutyPercent);     /* 0..100 */
status_t Pwm_SetFrequency(uint32_t freqHz);    /* 500, 1000 o 2000 */
uint8_t  Pwm_GetDuty(void);
uint32_t Pwm_GetFrequency(void);

#endif
