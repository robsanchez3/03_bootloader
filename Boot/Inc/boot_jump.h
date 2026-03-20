#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

#include <stdint.h>

/* Application starts right after the 128 KB bootloader region */
#define BOOTLOADER_SIZE   (128U * 1024U)
#define APP_BASE          (0x08000000U + BOOTLOADER_SIZE)   /* 0x08020000 */
#define APP_END           (0x081FFFFFU)

/* RAM boundaries used to validate the application MSP */
#define RAM_BASE          (0x20000000U)
#define RAM_END           (0x2026FFFFU)
#define RAM_TOP           (RAM_END + 1U)

/**
 * @brief  Check that a valid application is present at app_base.
 *         Verifies that the initial MSP is within RAM and that the
 *         reset vector points within the application flash region.
 * @param  app_base  Start address of the application (e.g. APP_BASE)
 * @retval 1  Application looks valid
 *         0  Application is not valid (erased or corrupt)
 */
int Boot_IsApplicationValid(uint32_t app_base);

/**
 * @brief  De-init HAL, relocate the vector table and jump to the
 *         application reset handler.  Does not return.
 * @param  app_base  Start address of the application (e.g. APP_BASE)
 */
void Boot_JumpToApplication(uint32_t app_base);

#endif /* BOOT_JUMP_H */
