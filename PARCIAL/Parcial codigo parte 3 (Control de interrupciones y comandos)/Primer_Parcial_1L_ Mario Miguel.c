#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "tick_app.h"
#include "led_app.h"
#include "uart_app.h"
#include "button_app.h"
#include "cmd_parser.h"

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    (void)Tick_Init();
    Led_Init();
    Uart_Init();
    Button_Init();

    Uart_Puts("\r\nPaso 3: parser. Escriba comandos (HELP, DUTY 50, ...)\r\n");
    Led_SetBlink(500U);

    uint32_t lastSvc = Tick_GetMs();

    for (;;)
    {
        uint32_t now = Tick_GetMs();
        cmd_t c;

        Led_Service(now);

        if (now != lastSvc)
        {
            lastSvc = now;
            button_event_t ev = Button_Service(now);
            if (ev == BTN_EVT_SHORT) { Uart_Puts("BOTON CORTA\r\n"); }
            if (ev == BTN_EVT_LONG)  { Uart_Puts("BOTON LARGA\r\n"); }
        }

        if (Cmd_Poll(&c))
        {
            if (c.id == CMD_ERROR)
            {
                Uart_Printf("%s\r\n", Cmd_ErrText(c.err));
            }
            else
            {
                Uart_Printf("OK id=%d arg=%lu\r\n", (int)c.id, (unsigned long)c.arg);
            }
        }

        Uart_TxService();
    }
}
