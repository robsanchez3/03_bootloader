#ifndef BOOT_OSPI_H
#define BOOT_OSPI_H

#include <stdint.h>
#include "stm32u5xx_hal.h"

typedef enum
{
    BOOT_OSPI_OK = 0,
    BOOT_OSPI_ERR_INIT,
    BOOT_OSPI_ERR_ERASE,
    BOOT_OSPI_ERR_PROGRAM,
    BOOT_OSPI_ERR_VERIFY,
    BOOT_OSPI_ERR_READ
} BootOspiResult_t;

/* Initialize OCTOSPI1 and leave the external MX25LM51245G in OPI STR mode,
 * ready for erase/program operations. */
BootOspiResult_t BootOspi_Init(void);

/* Progress callback: called with (current_sector, total_sectors). */
typedef void (*BootOspi_ProgressCb_t)(uint32_t current, uint32_t total);

/* Erase the required number of 64KB sectors starting at address 0.
 * len: bytes to erase (rounded up to 64KB sectors).
 * progress_cb: optional callback called after each sector (NULL to disable). */
BootOspiResult_t BootOspi_Erase(uint32_t len, BootOspi_ProgressCb_t progress_cb);

/* Program data into OSPI flash.
 * address: offset within OSPI flash (0-based).
 * data: source buffer.
 * len: number of bytes to write (internally split into 256-byte pages). */
BootOspiResult_t BootOspi_Program(uint32_t address, const uint8_t *data, uint32_t len);

/* Read data from OSPI flash.
 * address: offset within OSPI flash (0-based).
 * buffer: destination buffer.
 * len: number of bytes to read. */
BootOspiResult_t BootOspi_Read(uint32_t address, uint8_t *buffer, uint32_t len);

/* Switch OSPI back to memory-mapped mode so the image can be verified through
 * the OSPI address space and later used by the application. */
BootOspiResult_t BootOspi_EnableMemoryMapped(void);

/* Get OSPI HAL handle (for abort/re-init operations). */
OSPI_HandleTypeDef *BootOspi_GetHandle(void);

#endif /* BOOT_OSPI_H */
