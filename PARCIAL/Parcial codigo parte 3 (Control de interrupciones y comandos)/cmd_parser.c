#include "cmd_parser.h"
#include "uart_app.h"
#include <string.h>

#define LINE_MAX      63U      /* caracteres utiles; +1 para el NUL */
#define BYTE_BUDGET   32U      /* limite de trabajo por llamada */

typedef enum { NUM_OK, NUM_ARG, NUM_RANGE } num_t;

static char     s_line[LINE_MAX + 1U];
static uint32_t s_len = 0U;
static bool     s_discard = false;     /* descartando hasta el proximo fin de linea */
static bool     s_lastCR = false;
static uint32_t s_overflowCnt = 0U;

/* Valida TODA la cadena: solo digitos. '-' + digitos -> RANGE. Enormes -> RANGE. */
static num_t ParseUInt(const char *s, uint32_t *out)
{
    bool neg = false;
    bool big = false;
    uint32_t v = 0U;

    if (*s == '-')
    {
        neg = true;
        s++;
    }
    if (*s == '\0')
    {
        return NUM_ARG;
    }
    for (; *s != '\0'; s++)
    {
        if ((*s < '0') || (*s > '9'))
        {
            return NUM_ARG;            /* 20abc, +5, 1.5, etc. */
        }
        if (!big)
        {
            v = (v * 10U) + (uint32_t)(*s - '0');
            if (v > 1000000U)
            {
                big = true;            /* sin desbordar uint32 */
            }
        }
    }
    if (neg || big)
    {
        return NUM_RANGE;
    }
    *out = v;
    return NUM_OK;
}

static void SetErr(cmd_t *c, cmd_err_t e)
{
    c->id = CMD_ERROR;
    c->err = e;
}

/* Interpreta una linea ya completa (se modifica al separar tokens) */
static bool Interpret(char *line, cmd_t *c)
{
    char *tok[3] = {NULL, NULL, NULL};
    uint32_t n = 0U;
    char *p = line;

    while (*p != '\0')
    {
        if ((*p == ' ') || (*p == '\t'))
        {
            *p++ = '\0';
            continue;
        }
        if (n < 3U)
        {
            tok[n] = p;
        }
        n++;
        while ((*p != '\0') && (*p != ' ') && (*p != '\t'))
        {
            p++;
        }
    }

    if (n == 0U)
    {
        return false;                  /* linea vacia: se ignora */
    }

    c->id = CMD_NONE;
    c->arg = 0U;
    c->err = CMD_ERR_NONE;

    if (strcmp(tok[0], "HELP") == 0)
    {
        if (n != 1U) { SetErr(c, CMD_ERR_ARG); } else { c->id = CMD_HELP; }
    }
    else if (strcmp(tok[0], "STATUS") == 0)
    {
        if (n != 1U) { SetErr(c, CMD_ERR_ARG); } else { c->id = CMD_STATUS; }
    }
    else if (strcmp(tok[0], "PAUSE") == 0)
    {
        if (n != 1U) { SetErr(c, CMD_ERR_ARG); } else { c->id = CMD_PAUSE; }
    }
    else if (strcmp(tok[0], "RESUME") == 0)
    {
        if (n != 1U) { SetErr(c, CMD_ERR_ARG); } else { c->id = CMD_RESUME; }
    }
    else if (strcmp(tok[0], "MODE") == 0)
    {
        if (n != 2U)                              { SetErr(c, CMD_ERR_ARG); }
        else if (strcmp(tok[1], "MANUAL") == 0)   { c->id = CMD_MODE_MANUAL; }
        else if (strcmp(tok[1], "AUTO") == 0)     { c->id = CMD_MODE_AUTO; }
        else                                      { SetErr(c, CMD_ERR_ARG); }
    }
    else if (strcmp(tok[0], "STREAM") == 0)
    {
        if (n != 2U)                              { SetErr(c, CMD_ERR_ARG); }
        else if (strcmp(tok[1], "ON") == 0)       { c->id = CMD_STREAM_ON; }
        else if (strcmp(tok[1], "OFF") == 0)      { c->id = CMD_STREAM_OFF; }
        else                                      { SetErr(c, CMD_ERR_ARG); }
    }
    else if (strcmp(tok[0], "DUTY") == 0)
    {
        uint32_t v = 0U;
        if (n != 2U) { SetErr(c, CMD_ERR_ARG); }
        else
        {
            num_t r = ParseUInt(tok[1], &v);
            if (r == NUM_ARG)                        { SetErr(c, CMD_ERR_ARG); }
            else if ((r == NUM_RANGE) || (v > 100U)) { SetErr(c, CMD_ERR_RANGE); }
            else { c->id = CMD_DUTY; c->arg = v; }
        }
    }
    else if (strcmp(tok[0], "FREQ") == 0)
    {
        uint32_t v = 0U;
        if (n != 2U) { SetErr(c, CMD_ERR_ARG); }
        else
        {
            num_t r = ParseUInt(tok[1], &v);
            if (r == NUM_ARG) { SetErr(c, CMD_ERR_ARG); }
            else if ((r == NUM_RANGE) || ((v != 500U) && (v != 1000U) && (v != 2000U)))
            {
                SetErr(c, CMD_ERR_RANGE);
            }
            else { c->id = CMD_FREQ; c->arg = v; }
        }
    }
    else
    {
        SetErr(c, CMD_ERR_COMMAND);
    }
    return true;
}

/* Fin de linea detectado */
static bool EndOfLine(cmd_t *out)
{
    if (s_discard)
    {
        s_discard = false;
        s_len = 0U;
        SetErr(out, CMD_ERR_OVERFLOW);   /* resincroniza aqui, sin ejecutar fragmentos */
        out->arg = 0U;
        return true;
    }
    s_line[s_len] = '\0';
    s_len = 0U;
    return Interpret(s_line, out);
}

bool Cmd_Poll(cmd_t *out)
{
    uint8_t  b;
    uint32_t budget = BYTE_BUDGET;

    if (Uart_TakeRxLost())               /* se perdieron bytes: la linea afectada se descarta */
    {
        s_discard = true;
        s_len = 0U;
        s_overflowCnt++;
    }

    while ((budget > 0U) && Uart_ReadByte(&b))
    {
        budget--;

        if ((b == '\r') || (b == '\n'))
        {
            if ((b == '\n') && s_lastCR)  /* CRLF cuenta como una sola orden */
            {
                s_lastCR = false;
                continue;
            }
            s_lastCR = (b == '\r');
            if (EndOfLine(out))
            {
                return true;
            }
            continue;
        }

        s_lastCR = false;
        if (s_discard)
        {
            continue;
        }
        if (s_len < LINE_MAX)
        {
            s_line[s_len++] = (char)b;
        }
        else
        {
            s_discard = true;            /* linea de mas de 63 caracteres */
            s_len = 0U;
            s_overflowCnt++;
        }
    }
    return false;
}

const char *Cmd_ErrText(cmd_err_t e)
{
    switch (e)
    {
        case CMD_ERR_COMMAND:  return "ERR COMMAND";
        case CMD_ERR_ARG:      return "ERR ARG";
        case CMD_ERR_RANGE:    return "ERR RANGE";
        case CMD_ERR_OVERFLOW: return "ERR OVERFLOW";
        default:               return "ERR";
    }
}

uint32_t Cmd_OverflowCount(void) { return s_overflowCnt; }
