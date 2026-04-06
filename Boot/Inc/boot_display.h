#ifndef BOOT_DISPLAY_H
#define BOOT_DISPLAY_H

#include "stm32u5xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BOOT_DISPLAY_COLOR_NORMAL = 0,
    BOOT_DISPLAY_COLOR_BLUE,
    BOOT_DISPLAY_COLOR_GREEN,
    BOOT_DISPLAY_COLOR_YELLOW,
    BOOT_DISPLAY_COLOR_RED
} BootDisplayColor_t;

void BootDisplay_Init(void);
void BootDisplay_Clear(void);
void BootDisplay_ClearLine(uint32_t row);
void BootDisplay_WriteLine(uint32_t row, const char *text);
void BootDisplay_WriteLineColor(uint32_t row, const char *text, BootDisplayColor_t color);
void BootDisplay_Log(const char *text);
void BootDisplay_LogColor(const char *text, BootDisplayColor_t color);
void BootDisplay_ShowStatus(const char *line0, const char *line1);
void BootDisplay_ShowProgress(const char *label, uint32_t current, uint32_t total);
void BootDisplay_ClearProgress(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_DISPLAY_H */
