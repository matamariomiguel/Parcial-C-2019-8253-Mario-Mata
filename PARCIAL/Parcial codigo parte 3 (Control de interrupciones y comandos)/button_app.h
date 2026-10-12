#ifndef BUTTON_APP_H
#define BUTTON_APP_H

#include <stdint.h>

typedef enum { BTN_EVT_NONE = 0, BTN_EVT_SHORT, BTN_EVT_LONG } button_event_t;

void           Button_Init(void);
button_event_t Button_Service(uint32_t nowMs);   /* llamar una vez por cada ms */
uint32_t       Button_ShortCount(void);
uint32_t       Button_LongCount(void);

#endif
