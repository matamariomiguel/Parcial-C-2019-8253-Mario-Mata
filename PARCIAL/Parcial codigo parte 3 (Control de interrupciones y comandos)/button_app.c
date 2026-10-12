#include "button_app.h"
#include "app_config.h"
#include "app_time.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "fsl_common.h"
#include <stdbool.h>

#define DEBOUNCE_MS  30U
#define LONG_MS      1500U

static volatile uint32_t s_edgePending = 0U;

static bool     s_stable = false;     /* estado validado: true = presionado */
static bool     s_cand = false;       /* candidato en validacion */
static uint32_t s_candSince = 0U;
static uint32_t s_pressAt = 0U;       /* instante en que se valido la pulsacion */
static bool     s_longDone = false;
static uint32_t s_shortCnt = 0U;
static uint32_t s_longCnt = 0U;

/* ISR: limpia todos los flags del puerto (incluidos los ajenos) y avisa. Nada mas. */
void APP_BTN_IRQHandler(void)
{
    uint32_t flags = GPIO_GpioGetInterruptFlags(APP_BTN_GPIO);
    if (flags != 0U)
    {
        GPIO_GpioClearInterruptFlags(APP_BTN_GPIO, flags);
        if (flags & (1UL << APP_BTN_PIN))
        {
            s_edgePending = 1U;
        }
    }
    SDK_ISR_EXIT_BARRIER;
}

static uint32_t TakeEdge(void)
{
    uint32_t irq = DisableGlobalIRQ();
    uint32_t p = s_edgePending;
    s_edgePending = 0U;
    EnableGlobalIRQ(irq);
    return p;
}

void Button_Init(void)
{
    CLOCK_EnableClock(kCLOCK_GatePORT1);
    RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);

    /* GPIO (MUX 0), pull-up interno, buffer de entrada habilitado */
    APP_BTN_PORT->PCR[APP_BTN_PIN] = PORT_PCR_MUX(0U) | PORT_PCR_PE_MASK |
                                     PORT_PCR_PS_MASK | PORT_PCR_IBE_MASK;

    gpio_pin_config_t btn = {kGPIO_DigitalInput, 0U};
    GPIO_PortInit(APP_BTN_GPIO);
    GPIO_PinInit(APP_BTN_GPIO, APP_BTN_PIN, &btn);

    GPIO_GpioClearInterruptFlags(APP_BTN_GPIO, 1UL << APP_BTN_PIN);
    GPIO_SetPinInterruptConfig(APP_BTN_GPIO, APP_BTN_PIN, kGPIO_InterruptEitherEdge);
    EnableIRQ(APP_BTN_IRQn);

    /* Estado inicial coherente con el nivel actual del pin */
    s_cand = s_stable = (GPIO_PinRead(APP_BTN_GPIO, APP_BTN_PIN) == 0U);
}

button_event_t Button_Service(uint32_t now)
{
    button_event_t evt = BTN_EVT_NONE;
    bool raw = (GPIO_PinRead(APP_BTN_GPIO, APP_BTN_PIN) == 0U);   /* activo en bajo */

    /* Un flanco o un cambio de nivel reinicia los 30 ms */
    if (TakeEdge() || (raw != s_cand))
    {
        s_cand = raw;
        s_candSince = now;
    }
    else if ((s_cand != s_stable) && (Time_Elapsed(now, s_candSince) >= DEBOUNCE_MS))
    {
        s_stable = s_cand;                   /* cambio confirmado */
        if (s_stable)
        {
            s_pressAt = now;
            s_longDone = false;
        }
        else if (!s_longDone)
        {
            uint32_t d = Time_Elapsed(now, s_pressAt);   /* sobre estado validado */
            if ((d >= DEBOUNCE_MS) && (d < LONG_MS))
            {
                evt = BTN_EVT_SHORT;
                s_shortCnt++;
            }
        }
    }

    /* Larga: una sola vez al alcanzar 1500 ms, sin repeticion */
    if (s_stable && !s_longDone && (Time_Elapsed(now, s_pressAt) >= LONG_MS))
    {
        s_longDone = true;
        evt = BTN_EVT_LONG;
        s_longCnt++;
    }
    return evt;
}

uint32_t Button_ShortCount(void) { return s_shortCnt; }
uint32_t Button_LongCount(void)  { return s_longCnt; }
