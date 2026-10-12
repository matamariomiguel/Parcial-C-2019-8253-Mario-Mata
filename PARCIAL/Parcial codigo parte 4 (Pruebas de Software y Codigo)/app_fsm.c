#include "app_fsm.h"
#include "app_time.h"
#include "pwm_app.h"
#include "led_app.h"

#define AUTO_MIN       10U
#define AUTO_MAX       90U
#define AUTO_STEP_MS   20U

static fsm_mode_t s_mode = FSM_MODE_MANUAL;
static bool       s_paused = false;
static uint8_t    s_manualDuty = 25U;
static uint8_t    s_autoDuty = AUTO_MIN;
static int8_t     s_autoDir = 1;
static uint32_t   s_rampLast = 0U;
static uint32_t   s_shortIgnored = 0U;

static void UpdateLed(void)
{
    if (s_paused)                    { Led_SetBlink(125U); }
    else if (s_mode == FSM_MODE_AUTO) { Led_SetBlink(500U); }
    else                             { Led_SetSolid(); }
}

void Fsm_Init(uint32_t nowMs)
{
    s_mode = FSM_MODE_MANUAL;
    s_paused = false;
    s_manualDuty = 25U;
    s_rampLast = nowMs;
    (void)Pwm_SetDuty(s_manualDuty);
    UpdateLed();
}

fsm_result_t Fsm_SetMode(fsm_mode_t mode, uint32_t nowMs)
{
    if (s_paused)
    {
        return FSM_ERR_STATE;
    }
    s_mode = mode;
    if (mode == FSM_MODE_AUTO)
    {
        s_autoDuty = AUTO_MIN;           /* siempre desde 10 % ascendiendo */
        s_autoDir = 1;
        s_rampLast = nowMs;
        (void)Pwm_SetDuty(s_autoDuty);
    }
    else
    {
        (void)Pwm_SetDuty(s_manualDuty); /* restaura el ultimo duty manual */
    }
    UpdateLed();
    return FSM_OK;
}

fsm_result_t Fsm_SetDuty(uint8_t duty)
{
    if (s_paused || (s_mode != FSM_MODE_MANUAL))
    {
        return FSM_ERR_STATE;
    }
    if (Pwm_SetDuty(duty) == kStatus_Success)
    {
        s_manualDuty = duty;
    }
    return FSM_OK;
}

fsm_result_t Fsm_SetFreq(uint32_t freqHz)
{
    if (s_paused)
    {
        return FSM_ERR_STATE;
    }
    (void)Pwm_SetFrequency(freqHz);
    return FSM_OK;
}

fsm_result_t Fsm_Pause(uint32_t nowMs)
{
    (void)nowMs;
    if (!s_paused)                       /* ya pausado: OK sin cambiar lo guardado */
    {
        s_paused = true;                 /* conserva modo, duty y direccion */
        (void)Pwm_SetDuty(0U);
        UpdateLed();
    }
    return FSM_OK;
}

fsm_result_t Fsm_Resume(uint32_t nowMs)
{
    if (!s_paused)
    {
        return FSM_ERR_STATE;
    }
    s_paused = false;
    s_rampLast = nowMs;                  /* no recupera pasos del tiempo pausado */
    (void)Pwm_SetDuty((s_mode == FSM_MODE_AUTO) ? s_autoDuty : s_manualDuty);
    UpdateLed();
    return FSM_OK;
}

void Fsm_Service(uint32_t nowMs)
{
    if (s_paused || (s_mode != FSM_MODE_AUTO))
    {
        return;
    }
    if (!Time_Due(nowMs, &s_rampLast, AUTO_STEP_MS))
    {
        return;
    }
    int next = (int)s_autoDuty + s_autoDir;
    if (next >= (int)AUTO_MAX)      { next = (int)AUTO_MAX; s_autoDir = -1; }
    else if (next <= (int)AUTO_MIN) { next = (int)AUTO_MIN; s_autoDir = 1; }
    s_autoDuty = (uint8_t)next;
    (void)Pwm_SetDuty(s_autoDuty);
}

void Fsm_OnButton(button_event_t ev, uint32_t nowMs)
{
    if (ev == BTN_EVT_SHORT)
    {
        if (s_paused)
        {
            s_shortIgnored++;            /* corta en PAUSA se ignora */
            return;
        }
        (void)Fsm_SetMode((s_mode == FSM_MODE_MANUAL) ? FSM_MODE_AUTO : FSM_MODE_MANUAL, nowMs);
    }
    else if (ev == BTN_EVT_LONG)
    {
        if (s_paused) { (void)Fsm_Resume(nowMs); }
        else          { (void)Fsm_Pause(nowMs); }
    }
}

fsm_mode_t  Fsm_GetMode(void)            { return s_mode; }
bool        Fsm_IsPaused(void)           { return s_paused; }
uint8_t     Fsm_GetManualDuty(void)      { return s_manualDuty; }
uint32_t    Fsm_ShortIgnoredCount(void)  { return s_shortIgnored; }
const char *Fsm_ModeName(void)  { return (s_mode == FSM_MODE_AUTO) ? "AUTO" : "MANUAL"; }
const char *Fsm_StateName(void) { return s_paused ? "PAUSA" : Fsm_ModeName(); }
