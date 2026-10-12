#ifndef APP_FSM_H
#define APP_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "button_app.h"

typedef enum { FSM_MODE_MANUAL = 0, FSM_MODE_AUTO } fsm_mode_t;
typedef enum { FSM_OK = 0, FSM_ERR_STATE } fsm_result_t;

void Fsm_Init(uint32_t nowMs);
void Fsm_Service(uint32_t nowMs);                    /* rampa AUTO */
void Fsm_OnButton(button_event_t ev, uint32_t nowMs);

fsm_result_t Fsm_SetMode(fsm_mode_t mode, uint32_t nowMs);
fsm_result_t Fsm_SetDuty(uint8_t duty);
fsm_result_t Fsm_SetFreq(uint32_t freqHz);
fsm_result_t Fsm_Pause(uint32_t nowMs);
fsm_result_t Fsm_Resume(uint32_t nowMs);

fsm_mode_t  Fsm_GetMode(void);
bool        Fsm_IsPaused(void);
uint8_t     Fsm_GetManualDuty(void);
uint32_t    Fsm_ShortIgnoredCount(void);
const char *Fsm_ModeName(void);
const char *Fsm_StateName(void);

#endif
