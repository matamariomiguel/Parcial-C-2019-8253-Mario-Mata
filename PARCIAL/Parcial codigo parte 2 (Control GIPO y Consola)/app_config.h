#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "board.h"
#include "fsl_device_registers.h"

/* ===== Tabla de recursos ===== */

/* LED de estado: valores definidos por board.h del SDK de la placa */
#define APP_LED_GPIO         BOARD_LED_RED_GPIO
#define APP_LED_PIN          BOARD_LED_RED_GPIO_PIN
#define APP_LED_ON_LEVEL     LOGIC_LED_ON
#define APP_LED_OFF_LEVEL    LOGIC_LED_OFF
#define APP_LED_PORT         PORT3

/* Pulsador de usuario SW2 (wake-up), activo en bajo, pull-up */
#define APP_BTN_GPIO         GPIO1
#define APP_BTN_PORT         PORT1
#define APP_BTN_PIN          7U
#define APP_BTN_IRQn         GPIO1_IRQn
#define APP_BTN_IRQHandler   GPIO1_IRQHandler

/* UART de la consola (la misma que usa BOARD_InitDebugConsole) */
#define APP_UART             LPUART0
#define APP_UART_IRQn        LPUART0_IRQn
#define APP_UART_IRQHandler  LPUART0_IRQHandler

#endif
