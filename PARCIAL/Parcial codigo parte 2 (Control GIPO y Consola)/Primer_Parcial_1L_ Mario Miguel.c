#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "tick_app.h"
#include "led_app.h"
#include "uart_app.h"

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    (void)Tick_Init();
    Led_Init();
    Uart_Init();

    Uart_Puts("\r\nPaso 1: LED + UART OK. Escriba algo.\r\n");
    Led_SetBlink(500U);

    uint32_t lastPrint = Tick_GetMs();

    for (;;)
    {
        uint32_t now = Tick_GetMs();
        uint8_t b;

        Led_Service(now);

        if ((uint32_t)(now - lastPrint) >= 1000U)
        {
            lastPrint += 1000U;
            Uart_Printf("ms=%lu\r\n", (unsigned long)now);
        }

        while (Uart_ReadByte(&b))
        {
            char s[2] = { (char)b, '\0' };
            Uart_Puts(s);
        }
        Uart_TxService();
    }
}
