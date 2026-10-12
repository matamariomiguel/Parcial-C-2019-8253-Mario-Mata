#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    CMD_NONE = 0,
    CMD_HELP,
    CMD_STATUS,
    CMD_MODE_MANUAL,
    CMD_MODE_AUTO,
    CMD_DUTY,
    CMD_FREQ,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_STREAM_ON,
    CMD_STREAM_OFF,
    CMD_ERROR
} cmd_id_t;

typedef enum
{
    CMD_ERR_NONE = 0,
    CMD_ERR_COMMAND,
    CMD_ERR_ARG,
    CMD_ERR_RANGE,
    CMD_ERR_OVERFLOW
} cmd_err_t;

typedef struct
{
    cmd_id_t  id;
    uint32_t  arg;     /* valor de DUTY o FREQ ya validado */
    cmd_err_t err;     /* solo si id == CMD_ERROR */
} cmd_t;

/* No bloqueante. Devuelve true cuando una linea completa produjo un comando o un error. */
bool        Cmd_Poll(cmd_t *out);
const char *Cmd_ErrText(cmd_err_t e);
uint32_t    Cmd_OverflowCount(void);   /* lineas descartadas por sobrelongitud o perdida de RX */

#endif
