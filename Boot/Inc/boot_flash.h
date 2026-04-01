#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H

#include <stdint.h>

typedef enum
{
    BOOT_FLASH_OK = 0,
    BOOT_FLASH_ERR_UNLOCK,
    BOOT_FLASH_ERR_ERASE,
    BOOT_FLASH_ERR_PROGRAM,
    BOOT_FLASH_ERR_VERIFY,
    BOOT_FLASH_ERR_LOCK,
    BOOT_FLASH_ERR_ALIGNMENT,
    BOOT_FLASH_ERR_RANGE
} BootFlashResult_t;

/* Progress callback: called with (current_page, total_pages). */
typedef void (*BootFlash_ProgressCb_t)(uint32_t current, uint32_t total);

/* Erase the application area (APP_BASE to APP_BASE + size, rounded up to pages).
 * size: number of bytes to erase (will be rounded up to full 8KB pages).
 * progress_cb: optional callback called after each page (NULL to disable). */
BootFlashResult_t BootFlash_EraseAppArea(uint32_t size, BootFlash_ProgressCb_t progress_cb);

/* Program data into flash starting at the given address.
 * address: must be 16-byte aligned and within app area.
 * data:    source buffer (will be padded internally if len is not multiple of 16).
 * len:     number of bytes to write. */
BootFlashResult_t BootFlash_Program(uint32_t address, const uint8_t *data, uint32_t len);

/* Verify flash contents against a buffer.
 * Returns BOOT_FLASH_OK if contents match, BOOT_FLASH_ERR_VERIFY otherwise. */
BootFlashResult_t BootFlash_Verify(uint32_t address, const uint8_t *data, uint32_t len);

#endif /* BOOT_FLASH_H */
