#ifndef TICK_APP_H
#define TICK_APP_H

#include <stdint.h>

/* Conecta el reloj de CTIMER0 y arranca el tick de 1 ms.
 * Devuelve la frecuencia real del reloj del periferico (Hz). */
uint32_t Tick_Init(void);

/* Milisegundos desde el arranque. Use diferencias sin signo:
 * (uint32_t)(now - last) >= N */
uint32_t Tick_GetMs(void);

#endif
