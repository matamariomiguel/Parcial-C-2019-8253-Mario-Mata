#include "led_app.h"
#include "app_config.h"
#include "app_time.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_reset.h"

static bool     s_on = false;
static uint32_t s_period = 0U;           /* 0 = fijo */
static uint32_t s_last = 0U;
static bool     s_restart = true;

static void Led_Write(bool on)
{
    GPIO_PinWrite(APP_LED_GPIO, APP_LED_PIN,
                  on ? APP_LED_ON_LEVEL : APP_LED_OFF_LEVEL);
    s_on = on;
}

void Led_Init(void)
{
    CLOCK_EnableClock(kCLOCK_GatePORT3);
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);

    APP_LED_PORT->PCR[APP_LED_PIN] = PORT_PCR_MUX(0U);   /* GPIO */

    gpio_pin_config_t cfg = {kGPIO_DigitalOutput, APP_LED_OFF_LEVEL};
    GPIO_PortInit(APP_LED_GPIO);
    GPIO_PinInit(APP_LED_GPIO, APP_LED_PIN, &cfg);

    Led_SetSolid();
}

void Led_SetSolid(void)
{
    s_period = 0U;
    Led_Write(true);
}

void Led_SetBlink(uint32_t periodMs)
{
    s_period = periodMs;
    s_restart = true;                    /* la fase se reinicia en el siguiente Service */
}

void Led_Service(uint32_t nowMs)
{
    if (s_period == 0U)
    {
        return;
    }
    if (s_restart)
    {
        s_restart = false;
        s_last = nowMs;
        Led_Write(true);
        return;
    }
    if (Time_Due(nowMs, &s_last, s_period))
    {
        Led_Write(!s_on);
    }
}
