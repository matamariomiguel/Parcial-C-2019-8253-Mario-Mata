#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "tick_app.h"
#include "pwm_app.h"
#include "led_app.h"
#include "uart_app.h"
#include "button_app.h"
#include "app_fsm.h"
#include "console_app.h"

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    uint32_t tickClk = Tick_Init();
    uint32_t pwmClk = 0U;
    status_t st = Pwm_Init(&pwmClk);
    Led_Init();
    Uart_Init();
    Button_Init();

    uint32_t now = Tick_GetMs();
    Fsm_Init(now);
    Console_Init(now);

    Uart_Printf("\r\nFRDM-MCXA156 listo. CTIMER0=%lu Hz CTIMER1=%lu Hz PWM=%s\r\n",
                (unsigned long)tickClk, (unsigned long)pwmClk,
                (st == kStatus_Success) ? "OK" : "ERROR");
    Uart_Puts("Escriba HELP\r\n");

    uint32_t lastSvc = now;

    for (;;)
    {
        now = Tick_GetMs();

        if (now != lastSvc)                   /* antirrebote: una vez por ms */
        {
            lastSvc = now;
            Fsm_OnButton(Button_Service(now), now);
        }

        Fsm_Service(now);
        Led_Service(now);
        Console_Service(now);
        Uart_TxService();
    }
}
