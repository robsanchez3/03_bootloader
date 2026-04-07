/* Minimal stubs to compile boot_manifest.c on a PC (no HAL). */
#ifndef TEST_STUBS_H
#define TEST_STUBS_H

#include <stdint.h>

/* Stub boot_crc.h — prevent real header from being included */
#ifndef BOOT_CRC_H
#define BOOT_CRC_H
static inline void BootCrc32_Init(void) {}
static inline uint32_t BootCrc32_Compute(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    (void)crc; (void)buf; (void)len;
    return 0U;
}
static inline void BootCrc32_DeInit(void) {}
#endif

#endif /* TEST_STUBS_H */
