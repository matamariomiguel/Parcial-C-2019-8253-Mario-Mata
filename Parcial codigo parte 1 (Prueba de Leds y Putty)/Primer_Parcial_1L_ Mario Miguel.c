#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_clock.h"
#include "tick_app.h"
#include "pwm_app.h"

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("\r\n--- FRDM-MCXA156: Control de Iluminacion con PWM ---\r\n");

    uint32_t tickClk = Tick_Init();
    PRINTF("CTIMER0 clk: %lu Hz, tick 1 ms\r\n", (unsigned long)tickClk);

    uint32_t pwmClk = 0U;
    status_t st = Pwm_Init(&pwmClk);
    PRINTF("CTIMER1 clk: %lu Hz\r\n", (unsigned long)pwmClk);
    if (st == kStatus_Success)
    {
        PRINTF("PWM OK en P3_12 (1000 Hz, 25%%)\r\n");
    }
    else
    {
        PRINTF("Error al inicializar PWM: %ld\r\n", (long)st);
    }

    uint32_t lastBeat = Tick_GetMs();
    for (;;)
    {
        uint32_t now = Tick_GetMs();
        if ((uint32_t)(now - lastBeat) >= 1000U)
        {
            lastBeat += 1000U;
            PRINTF("ms=%lu duty=%u freq=%lu\r\n",
                   (unsigned long)now,
                   (unsigned)Pwm_GetDuty(),
                   (unsigned long)Pwm_GetFrequency());
        }

        /* Aqui iran: antirrebote, UART RX/parser, maquina de estados */
    }
}
