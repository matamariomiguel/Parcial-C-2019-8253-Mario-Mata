#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdint.h>
#include <stdbool.h>

/* Tiempo transcurrido, correcto aunque el contador se desborde */
static inline uint32_t Time_Elapsed(uint32_t now, uint32_t since)
{
    return (uint32_t)(now - since);
}

/* true cada 'period' ms. Avanza 'last' en pasos exactos (sin deriva)
 * y se resincroniza si el programa se atrasa mas de un periodo. */
static inline bool Time_Due(uint32_t now, uint32_t *last, uint32_t period)
{
    if (Time_Elapsed(now, *last) < period)
    {
        return false;
    }
    *last += period;
    if (Time_Elapsed(now, *last) >= period)
    {
        *last = now;
    }
    return true;
}

#endif
