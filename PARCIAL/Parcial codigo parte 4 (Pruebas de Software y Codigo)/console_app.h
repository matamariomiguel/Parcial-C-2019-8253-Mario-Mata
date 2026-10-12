#ifndef CONSOLE_APP_H
#define CONSOLE_APP_H

#include <stdint.h>

void Console_Init(uint32_t nowMs);
void Console_Service(uint32_t nowMs);   /* parser, respuestas y telemetria */

#endif
