/* boot_crypto.c — AES-256-CBC decryption and device key derivation.
   Ported from O3dev_edt_00/Drivers/Crypto/crypto.c for manifest.sig
   authorization of console USB updates (Plan_Cifrado_Bootloader_Consola.txt).
   Uses software AES (sw_aes.c) — the STM32U599 has no AES hardware peripheral. */

#include "boot_crypto.h"
#include "secrets.h"
#include "sw_aes.h"
#include <string.h>

/* Maximum size crypto_decrypt() can handle in one call (bytes, including IV).
   Only used for manifest.sig (128 bytes on disk: IV(16) + AES-CBC(magic(4) +
   boot_sig_payload_t(96) + PKCS7 padding)) — keep just above that; it backs
   a static buffer, so oversizing it wastes RAM the linker must reserve. */
#define CRYPTO_MAX_FILE_SIZE  256u

/* ── Public API ───────────────────────────────────────────────────── */

void crypto_derive_key(const uint8_t *org_key,
                       const uint8_t *uid_12,
                       uint8_t       *out_key)
{
    /* KDF: device_key = AES-256-ECB( key=org_key, data=uid_padded_to_32_bytes ).
       The UID (12 bytes) is zero-padded to two AES blocks (32 bytes);
       each block is encrypted independently to produce the 32-byte device key. */
    uint8_t uid_padded[CRYPTO_KEY_SIZE] = {0};
    memcpy(uid_padded, uid_12, CRYPTO_UID_SIZE);
    sw_aes256_ecb_encrypt(org_key, uid_padded,      out_key);
    sw_aes256_ecb_encrypt(org_key, uid_padded + 16, out_key + 16);
}

bool crypto_decrypt(const uint8_t *in,  uint32_t  in_size,
                    const uint8_t *key,
                    uint8_t       *out, uint32_t *out_size)
{
    if (in_size <= CRYPTO_IV_SIZE)             return false;
    if (in_size > CRYPTO_MAX_FILE_SIZE)        return false;

    uint32_t cipher_size = in_size - CRYPTO_IV_SIZE;
    if (cipher_size % 16u != 0u)               return false;

    const uint8_t *iv         = in;
    const uint8_t *ciphertext = in + CRYPTO_IV_SIZE;

    /* Decrypt into a static temporary buffer */
    static uint8_t tmp[CRYPTO_MAX_FILE_SIZE];
    sw_aes256_cbc_decrypt(key, iv, ciphertext, tmp, cipher_size);

    /* Verify magic header (CRYPTO_MAGIC = 0x5344434C = "SDCL") */
    uint32_t magic = 0u;
    memcpy(&magic, tmp, sizeof(magic));
    if (magic != CRYPTO_MAGIC) return false;

    /* Remove PKCS7 padding */
    uint8_t pad = tmp[cipher_size - 1u];
    if (pad == 0u || pad > 16u) return false;

    uint32_t plain_size = cipher_size - pad - (uint32_t)sizeof(magic);
    if (plain_size > *out_size) return false;

    memcpy(out, tmp + sizeof(magic), plain_size);
    *out_size = plain_size;

    return true;
}

bool crypto_decrypt_sig(const uint8_t *sig_buf, uint32_t sig_size,
                        const uint8_t *uid_12,
                        boot_sig_payload_t *out_payload)
{
    uint32_t out_size = (uint32_t)sizeof(boot_sig_payload_t);

    /* Try device key first (USB de equipo), then org key (USB maestro). */
    uint8_t dev_key[CRYPTO_KEY_SIZE];
    crypto_derive_key(CRYPTO_ORG_KEY, uid_12, dev_key);
    if (crypto_decrypt(sig_buf, sig_size, dev_key, (uint8_t*)out_payload, &out_size))
        return out_size == (uint32_t)sizeof(boot_sig_payload_t);

    out_size = (uint32_t)sizeof(boot_sig_payload_t);
    if (crypto_decrypt(sig_buf, sig_size, CRYPTO_ORG_KEY, (uint8_t*)out_payload, &out_size))
        return out_size == (uint32_t)sizeof(boot_sig_payload_t);

    return false;
}
