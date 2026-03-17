#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

/* -----------------------------------------------------------------------
 * GPIO pins used by the bootloader
 * ----------------------------------------------------------------------- */

/* VBUS power switch — active LOW (RESET=VBUS ON, SET=VBUS OFF) */
#define FS_PW_SW_Pin        GPIO_PIN_15
#define FS_PW_SW_GPIO_Port  GPIOI

/* Status LED (RTC_TS_LED) — used to signal recovery mode */
#define BOOT_LED_Pin        GPIO_PIN_14
#define BOOT_LED_GPIO_Port  GPIOB

/* -----------------------------------------------------------------------
 * Error handler
 * ----------------------------------------------------------------------- */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
