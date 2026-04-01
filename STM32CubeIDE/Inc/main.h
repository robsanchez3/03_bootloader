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

/* LCD control pins */
#define LCD_BL_PWM_Pin       GPIO_PIN_5
#define LCD_BL_PWM_GPIO_Port GPIOE

#define LCD_RESET_Pin        GPIO_PIN_15
#define LCD_RESET_GPIO_Port  GPIOH

#define LCD_CTRL_Pin         GPIO_PIN_13
#define LCD_CTRL_GPIO_Port   GPIOH

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
