#include "tick_app.h"
#include "fsl_ctimer.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "fsl_common.h"

#define APP_TICK_TIMER      CTIMER0
#define APP_TICK_IRQn       CTIMER0_IRQn
#define APP_TICK_INSTANCE   0U

static volatile uint32_t g_ms = 0U;

static void Tick_Callback(uint32_t flags)
{
    if (flags & kCTIMER_Match0Flag)
    {
        ++g_ms;
    }
}

static ctimer_callback_t s_tickCallbacks[] = { Tick_Callback };

uint32_t Tick_Init(void)
{
    ctimer_config_t cfg;
    ctimer_match_config_t matchCfg = {0};

    /* Confirme estos nombres en el ejemplo ctimer de su SDK */
    CLOCK_SetClockDiv(kCLOCK_DivCTIMER0, 1U);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER0);
    RESET_ReleasePeripheralReset(kCTIMER0_RST_SHIFT_RSTn);

    uint32_t timerClockHz = CLOCK_GetCTimerClkFreq(APP_TICK_INSTANCE);

    CTIMER_GetDefaultConfig(&cfg);
    cfg.prescale = 0U;
    CTIMER_Init(APP_TICK_TIMER, &cfg);

    matchCfg.matchValue         = (timerClockHz / 1000U) - 1U;
    matchCfg.enableCounterReset = true;
    matchCfg.enableCounterStop  = false;
    matchCfg.outControl         = kCTIMER_Output_NoAction;
    matchCfg.outPinInitState    = false;
    matchCfg.enableInterrupt    = true;

    CTIMER_RegisterCallBack(APP_TICK_TIMER, s_tickCallbacks, kCTIMER_SingleCallback);
    CTIMER_SetupMatch(APP_TICK_TIMER, kCTIMER_Match_0, &matchCfg);

    EnableIRQ(APP_TICK_IRQn);
    CTIMER_StartTimer(APP_TICK_TIMER);

    return timerClockHz;
}

uint32_t Tick_GetMs(void)
{
    uint32_t msVal;
    uint32_t irqState = DisableGlobalIRQ();
    msVal = g_ms;
    EnableGlobalIRQ(irqState);
    return msVal;
}
