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
 * @brief  Store CRC32 and size of both app images in RTC backup registers
 *         after a successful programming cycle.  Values survive resets.
 * @param  int_crc32   CRC32 of app_int image in internal flash
 * @param  int_size    Size in bytes of app_int image
 * @param  ospi_crc32  CRC32 of app_ospi image in OSPI flash
 * @param  ospi_size   Size in bytes of app_ospi image
 */
void Boot_StoreAppCrc(uint32_t int_crc32, uint32_t int_size,
                      uint32_t ospi_crc32, uint32_t ospi_size);

/**
 * @brief  Verify that the app images in flash match the CRC stored in
 *         backup registers by a previous successful programming cycle.
 * @retval  1  CRC matches (or no stored CRC — first boot)
 *          0  CRC mismatch — app is corrupt
 */
int Boot_VerifyAppCrc(void);

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
