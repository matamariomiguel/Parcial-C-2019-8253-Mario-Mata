#include "pwm_app.h"
#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "fsl_device_registers.h"

#define APP_PWM_TIMER        CTIMER1
#define APP_PWM_INSTANCE     1U
#define APP_PWM_PERIOD_CH    kCTIMER_Match_3
#define APP_PWM_OUTPUT_CH    kCTIMER_Match_2
#define APP_PWM_PIN_PORT     PORT3
#define APP_PWM_PIN_GPIO     GPIO3
#define APP_PWM_PIN_NUM      12U
#define APP_PWM_PIN_MUX      4U

static uint8_t  s_duty   = 25U;
static uint32_t s_freqHz = 1000U;
static uint32_t s_clkHz  = 0U;
static bool     s_pinIsGpio = false;

static bool Pwm_FreqAllowed(uint32_t f)
{
    return (f == 500U) || (f == 1000U) || (f == 2000U);
}

/* Pin controlado por el CTIMER (PWM) */
static void Pin_ToTimer(void)
{
    APP_PWM_PIN_PORT->PCR[APP_PWM_PIN_NUM] =
        (APP_PWM_PIN_PORT->PCR[APP_PWM_PIN_NUM] & ~PORT_PCR_MUX_MASK) |
        PORT_PCR_MUX(APP_PWM_PIN_MUX);
    s_pinIsGpio = false;
}

/* Pin como GPIO con nivel fijo: para 0 % (bajo) y 100 % (alto).
 * Se fija el nivel ANTES de cambiar el mux, para no generar pulsos. */
static void Pin_ToGpio(bool high)
{
    gpio_pin_config_t cfg = {kGPIO_DigitalOutput, high ? 1U : 0U};
    GPIO_PinInit(APP_PWM_PIN_GPIO, APP_PWM_PIN_NUM, &cfg);
    APP_PWM_PIN_PORT->PCR[APP_PWM_PIN_NUM] =
        (APP_PWM_PIN_PORT->PCR[APP_PWM_PIN_NUM] & ~PORT_PCR_MUX_MASK) |
        PORT_PCR_MUX(0U);
    s_pinIsGpio = true;
}

/* El timer nunca se programa con 0 % ni 100 %: usa 1..99 */
static uint8_t TimerDuty(void)
{
    if (s_duty == 0U)   { return 1U; }
    if (s_duty == 100U) { return 99U; }
    return s_duty;
}

static status_t Pwm_Apply(void)
{
    CTIMER_StopTimer(APP_PWM_TIMER);
    CTIMER_Reset(APP_PWM_TIMER);

    status_t rc = CTIMER_SetupPwm(APP_PWM_TIMER, APP_PWM_PERIOD_CH,
                                  APP_PWM_OUTPUT_CH, TimerDuty(), s_freqHz,
                                  s_clkHz, false);
    if (rc == kStatus_Success)
    {
        CTIMER_StartTimer(APP_PWM_TIMER);
    }
    return rc;
}

status_t Pwm_Init(uint32_t *clockHz)
{
    ctimer_config_t config;

    CLOCK_EnableClock(kCLOCK_GatePORT3);
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);

    Pin_ToTimer();

    CLOCK_SetClockDiv(kCLOCK_DivCTIMER1, 1U);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER1);
    RESET_ReleasePeripheralReset(kCTIMER1_RST_SHIFT_RSTn);

    s_clkHz = CLOCK_GetCTimerClkFreq(APP_PWM_INSTANCE);
    if (clockHz != NULL)
    {
        *clockHz = s_clkHz;
    }

    CTIMER_GetDefaultConfig(&config);
    config.prescale = 0U;
    CTIMER_Init(APP_PWM_TIMER, &config);

    s_duty   = 25U;
    s_freqHz = 1000U;
    s_freqHz = 2U;
    return Pwm_Apply();
}

status_t Pwm_SetDuty(uint8_t dutyPercent)
{
    if (dutyPercent > 100U)
    {
        return kStatus_InvalidArgument;
    }
    s_duty = dutyPercent;

    if ((dutyPercent == 0U) || (dutyPercent == 100U))
    {
        Pin_ToGpio(dutyPercent == 100U);      /* nivel constante, sin pulsos */
        return kStatus_Success;
    }

    status_t rc = Pwm_Apply();                /* reprograma periodo y duty */
    if (s_pinIsGpio)
    {
        Pin_ToTimer();                        /* devuelve el pin al CTIMER */
    }
    return rc;
}

status_t Pwm_SetFrequency(uint32_t freqHz)
{
    if (!Pwm_FreqAllowed(freqHz))
    {
        return kStatus_InvalidArgument;
    }
    s_freqHz = freqHz;
    return Pwm_Apply();
}

uint8_t  Pwm_GetDuty(void)      { return s_duty; }
uint32_t Pwm_GetFrequency(void) { return s_freqHz; }
