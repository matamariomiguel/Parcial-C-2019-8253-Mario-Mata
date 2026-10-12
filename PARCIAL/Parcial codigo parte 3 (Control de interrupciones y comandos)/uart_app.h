#ifndef UART_APP_H
#define UART_APP_H

#include <stdint.h>
#include <stdbool.h>

void     Uart_Init(void);
bool     Uart_ReadByte(uint8_t *b);      /* no bloqueante */
bool     Uart_TakeRxLost(void);          /* true si se perdieron bytes de RX */
void     Uart_Puts(const char *s);       /* encola; si no cabe, descarta y cuenta */
void     Uart_Printf(const char *fmt, ...);
void     Uart_TxService(void);           /* vaciar cola sin bloquear (llamar en main) */

uint32_t Uart_RxOverrunCount(void);      /* overrun de hardware */
uint32_t Uart_RxLostCount(void);         /* bytes perdidos por buffer circular lleno */
uint32_t Uart_TxDropCount(void);

#endif
