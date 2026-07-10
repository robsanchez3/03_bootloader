#ifndef BOOT_CRYPTO_H
#define BOOT_CRYPTO_H

#include <stdint.h>
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────── */

#define CRYPTO_IV_SIZE   16u   /* AES block size — prepended to every encrypted file */
#define CRYPTO_KEY_SIZE  32u   /* AES-256 key length in bytes */
#define CRYPTO_UID_SIZE  12u   /* STM32 UID: HAL_GetUIDw0/1/2 = 3 × 4 bytes */

/* ── API ────────────────────────────────────────────────────────────── */

/* Derive a device-specific 32-byte key from the organisation key and the
   STM32 UID (Unique Identifier — 96-bit value, unique per chip, factory-programmed).
   Uses AES-256-ECB: device_key = AES(key=org_key, data=uid_padded_to_32_bytes).
   org_key : CRYPTO_KEY_SIZE bytes  (CRYPTO_ORG_KEY from secrets.h)
   uid_12  : CRYPTO_UID_SIZE bytes  (HAL_GetUIDw0/1/2 concatenated)
   out_key : CRYPTO_KEY_SIZE bytes output */
void crypto_derive_key(const uint8_t *org_key,
                       const uint8_t *uid_12,
                       uint8_t       *out_key);

/* Decrypt an AES-256-CBC buffer and verify the magic header (CRYPTO_MAGIC).
   File layout on disk:
     [ IV (16 bytes) ][ AES-CBC( MAGIC(4) + content + PKCS7 padding ) ]
   in      : full file buffer — IV followed by ciphertext
   in_size : total bytes including IV
   key     : CRYPTO_KEY_SIZE bytes
   out     : caller-allocated buffer, receives plaintext without magic header
   out_size: on entry maximum capacity of out; on exit actual bytes written
   Returns true if decryption succeeded and CRYPTO_MAGIC matched. */
bool crypto_decrypt(const uint8_t *in,  uint32_t  in_size,
                    const uint8_t *key,
                    uint8_t       *out, uint32_t *out_size);

/* ── Firmware authorization for the console update package (manifest.sig),
   see Plan_Cifrado_Bootloader_Consola.txt ───────────────────────────── */

#define SIG_TYPE_MASTER  0u   /* manifest.sig encrypted with CRYPTO_ORG_KEY — any device   */
#define SIG_TYPE_DEVICE  1u   /* manifest.sig encrypted with the device key — one UID only */

/* Plaintext payload carried inside an encrypted manifest.sig file (96 bytes,
   after the CRYPTO_MAGIC header already handled by crypto_decrypt()). */
typedef struct __attribute__((packed)) {
    char     product[20];          /* informational / defence in depth               */
    uint32_t app_int_size;
    uint8_t  app_int_sha256[32];   /* SHA-256 of app_int.bin                          */
    uint32_t app_ospi_size;
    uint8_t  app_ospi_sha256[32];  /* SHA-256 of app_ospi.bin                         */
    uint8_t  type;                 /* SIG_TYPE_MASTER or SIG_TYPE_DEVICE              */
    uint8_t  reserved[3];
} boot_sig_payload_t;

/* Decrypt a manifest.sig file buffer and parse it into a boot_sig_payload_t.
   Tries the device key (KDF from uid_12) first, then the org key.
   sig_buf     : full manifest.sig file content — IV followed by ciphertext
   sig_size    : total bytes including IV
   uid_12      : 12-byte STM32 UID (HAL_GetUIDw0/1/2 concatenated)
   out_payload : receives the decoded struct on success
   Returns true if decryption succeeded (magic verified) and the decrypted
   payload is exactly sizeof(boot_sig_payload_t) bytes. Does NOT check
   product/sizes/hashes against the real files — caller does that. */
bool crypto_decrypt_sig(const uint8_t *sig_buf, uint32_t sig_size,
                        const uint8_t *uid_12,
                        boot_sig_payload_t *out_payload);

#endif /* BOOT_CRYPTO_H */
