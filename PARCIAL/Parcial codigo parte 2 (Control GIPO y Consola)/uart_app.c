#include "uart_app.h"
#include "app_config.h"
#include "fsl_lpuart.h"
#include "fsl_common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define RX_SIZE  128U              /* potencia de 2, minimo exigido */
#define RX_MASK  (RX_SIZE - 1U)
#define TX_SIZE  1024U
#define TX_MASK  (TX_SIZE - 1U)

static uint8_t           s_rx[RX_SIZE];
static volatile uint32_t s_rxHead = 0U;   /* escribe la ISR */
static volatile uint32_t s_rxTail = 0U;   /* escribe main   */
static volatile uint32_t s_rxLostFlag = 0U;
static volatile uint32_t s_rxLost = 0U;
static volatile uint32_t s_rxOverrun = 0U;

static uint8_t  s_tx[TX_SIZE];            /* solo se toca desde main */
static uint32_t s_txHead = 0U;
static uint32_t s_txTail = 0U;
static uint32_t s_txDrop = 0U;

/* ISR: solo recibe, limpia errores y guarda en el buffer circular */
void APP_UART_IRQHandler(void)
{
    uint32_t flags = LPUART_GetStatusFlags(APP_UART);
    uint32_t err = flags & (kLPUART_RxOverrunFlag | kLPUART_NoiseErrorFlag |
                            kLPUART_FramingErrorFlag | kLPUART_ParityErrorFlag);
    if (err != 0U)
    {
        if (err & kLPUART_RxOverrunFlag)
        {
            s_rxOverrun++;
            s_rxLostFlag = 1U;
        }
        LPUART_ClearStatusFlags(APP_UART, err);
    }

    while (LPUART_GetStatusFlags(APP_UART) & kLPUART_RxDataRegFullFlag)
    {
        uint8_t  b    = LPUART_ReadByte(APP_UART);
        uint32_t next = (s_rxHead + 1U) & RX_MASK;
        if (next != s_rxTail)
        {
            s_rx[s_rxHead] = b;
            s_rxHead = next;
        }
        else
        {
            s_rxLost++;            /* buffer lleno: descarta el byte */
            s_rxLostFlag = 1U;
        }
    }
    SDK_ISR_EXIT_BARRIER;
}

void Uart_Init(void)
{
    s_rxHead = s_rxTail = 0U;
    LPUART_ClearStatusFlags(APP_UART, kLPUART_RxOverrunFlag | kLPUART_NoiseErrorFlag |
                                      kLPUART_FramingErrorFlag | kLPUART_ParityErrorFlag);
    LPUART_EnableInterrupts(APP_UART, kLPUART_RxDataRegFullInterruptEnable |
                                      kLPUART_RxOverrunInterruptEnable);
    EnableIRQ(APP_UART_IRQn);
}

bool Uart_ReadByte(uint8_t *b)
{
    if (s_rxTail == s_rxHead)
    {
        return false;
    }
    *b = s_rx[s_rxTail];
    s_rxTail = (s_rxTail + 1U) & RX_MASK;
    return true;
}

bool Uart_TakeRxLost(void)
{
    uint32_t irq = DisableGlobalIRQ();
    bool lost = (s_rxLostFlag != 0U);
    s_rxLostFlag = 0U;
    EnableGlobalIRQ(irq);
    return lost;
}

void Uart_Puts(const char *s)
{
    uint32_t n = (uint32_t)strlen(s);
    uint32_t used = (s_txHead - s_txTail) & TX_MASK;
    uint32_t freeBytes = TX_SIZE - 1U - used;

    if (n > freeBytes)
    {
        s_txDrop++;               /* no cabe el mensaje completo: se descarta */
        return;
    }
    for (uint32_t i = 0U; i < n; i++)
    {
        s_tx[s_txHead] = (uint8_t)s[i];
        s_txHead = (s_txHead + 1U) & TX_MASK;
    }
}

void Uart_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Uart_Puts(buf);
}

/* Envia solo lo que el hardware acepta ahora; nunca espera */
void Uart_TxService(void)
{
    uint32_t n = 0U;
    while ((s_txHead != s_txTail) && (n < 16U) &&
           (LPUART_GetStatusFlags(APP_UART) & kLPUART_TxDataRegEmptyFlag))
    {
        LPUART_WriteByte(APP_UART, s_tx[s_txTail]);
        s_txTail = (s_txTail + 1U) & TX_MASK;
        n++;
    }
}

uint32_t Uart_RxOverrunCount(void) { return s_rxOverrun; }
uint32_t Uart_RxLostCount(void)    { return s_rxLost; }
uint32_t Uart_TxDropCount(void)    { return s_txDrop; }
