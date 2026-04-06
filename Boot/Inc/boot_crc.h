#ifndef BOOT_CRC_H
#define BOOT_CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize the STM32 hardware CRC peripheral.
 *         Safe to call multiple times — only initializes once.
 */
void BootCrc32_Init(void);

/**
 * @brief  Compute CRC32 (IEEE 802.3) using the hardware CRC peripheral.
 * @param  crc  Pass 0 to start a new calculation, or the previous return
 *              value to accumulate over multiple buffers.
 * @param  buf  Data buffer
 * @param  len  Number of bytes
 * @retval CRC32 value (with standard final XOR)
 */
uint32_t BootCrc32_Compute(uint32_t crc, const uint8_t *buf, uint32_t len);

/**
 * @brief  De-initialize the CRC peripheral and disable its clock.
 */
void BootCrc32_DeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_CRC_H */
