#include "console_app.h"
#include "cmd_parser.h"
#include "app_fsm.h"
#include "app_time.h"
#include "pwm_app.h"
#include "button_app.h"
#include "uart_app.h"
#include <stdbool.h>

#define UL(x) ((unsigned long)(x))

static bool     s_stream = false;
static uint32_t s_streamLast = 0U;
static uint32_t s_errCount = 0U;

static void ReplyErr(const char *txt)
{
    s_errCount++;
    Uart_Printf("%s\r\n", txt);
}

static void Status(uint32_t now)
{
    Uart_Printf("OK MODE=%s STATE=%s FREQ=%lu DUTY=%u MANUAL=%u UP=%lu "
                "SHORT=%lu LONG=%lu ERR=%lu OVF=%lu RXOVR=%lu RXLOST=%lu TXDROP=%lu\r\n",
                Fsm_ModeName(), Fsm_StateName(),
                UL(Pwm_GetFrequency()), (unsigned)Pwm_GetDuty(),
                (unsigned)Fsm_GetManualDuty(), UL(now),
                UL(Button_ShortCount()), UL(Button_LongCount()),
                UL(s_errCount), UL(Cmd_OverflowCount()),
                UL(Uart_RxOverrunCount()), UL(Uart_RxLostCount()),
                UL(Uart_TxDropCount()));
}

static void Execute(const cmd_t *c, uint32_t now)
{
    switch (c->id)
    {
        case CMD_HELP:
            Uart_Puts("OK\r\nHELP | STATUS | MODE MANUAL | MODE AUTO | DUTY n (0-100)\r\n"
                      "FREQ n (500,1000,2000) | PAUSE | RESUME | STREAM ON | STREAM OFF\r\n");
            break;

        case CMD_STATUS:
            Status(now);
            break;

        case CMD_MODE_MANUAL:
            if (Fsm_SetMode(FSM_MODE_MANUAL, now) == FSM_OK) { Uart_Puts("OK MODE MANUAL\r\n"); }
            else { ReplyErr("ERR STATE"); }
            break;

        case CMD_MODE_AUTO:
            if (Fsm_SetMode(FSM_MODE_AUTO, now) == FSM_OK) { Uart_Puts("OK MODE AUTO\r\n"); }
            else { ReplyErr("ERR STATE"); }
            break;

        case CMD_DUTY:
            if (Fsm_SetDuty((uint8_t)c->arg) == FSM_OK) { Uart_Printf("OK DUTY %lu\r\n", UL(c->arg)); }
            else { ReplyErr("ERR STATE"); }
            break;

        case CMD_FREQ:
            if (Fsm_SetFreq(c->arg) == FSM_OK) { Uart_Printf("OK FREQ %lu\r\n", UL(c->arg)); }
            else { ReplyErr("ERR STATE"); }
            break;

        case CMD_PAUSE:
            (void)Fsm_Pause(now);
            Uart_Puts("OK\r\n");
            break;

        case CMD_RESUME:
            if (Fsm_Resume(now) == FSM_OK) { Uart_Puts("OK\r\n"); }
            else { ReplyErr("ERR STATE"); }
            break;

        case CMD_STREAM_ON:
            s_stream = true;
            s_streamLast = now;
            Uart_Puts("OK\r\n");
            break;

        case CMD_STREAM_OFF:
            s_stream = false;
            Uart_Puts("OK\r\n");
            break;

        default:
            break;
    }
}

void Console_Init(uint32_t nowMs)
{
    s_stream = false;                    /* telemetria desactivada al inicio */
    s_streamLast = nowMs;
}

void Console_Service(uint32_t nowMs)
{
    cmd_t c;

    if (Cmd_Poll(&c))
    {
        if (c.id == CMD_ERROR)
        {
            ReplyErr(Cmd_ErrText(c.err));
        }
        else
        {
            Execute(&c, nowMs);
        }
    }

    if (s_stream && Time_Due(nowMs, &s_streamLast, 1000U))
    {
        Uart_Printf("T UP=%lu MODE=%s STATE=%s FREQ=%lu DUTY=%u\r\n",
                    UL(nowMs), Fsm_ModeName(), Fsm_StateName(),
                    UL(Pwm_GetFrequency()), (unsigned)Pwm_GetDuty());
    }
}
