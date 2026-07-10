#ifndef SECRETS_H
#define SECRETS_H

#include <stdint.h>

/* INSTRUCTIONS
   ------------
   1. Copy this file to Boot/Inc/secrets.h
   2. Replace the placeholder key bytes with the real organisation key
      (must match the CRYPTO_ORG_KEY in O3dev_edt_00/Drivers/Crypto/secrets.h —
      see Plan_Cifrado_Bootloader_Consola.txt, same key is reused)
   3. Replace CRYPTO_MAGIC with the real magic value
   4. secrets.h is excluded from the repository (.gitignore) — never commit it

   See Plan_Cifrado_Bootloader_Consola.txt for the full encryption scheme. */


/* Uncomment to enable manifest.sig authorization for USB console updates
   (Plan_Cifrado_Bootloader_Consola.txt). When commented out, the bootloader
   behaves exactly as before: only CRC32 + product/hw_revision are checked. */
#define ENABLE_CRYPTO

/* Organisation AES-256 master key (32 bytes).
   Replace placeholder zeros with the real key obtained from the organisation. */
static const uint8_t CRYPTO_ORG_KEY[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Magic header — first 4 bytes of every decrypted payload.
   Used to verify that decryption succeeded with the correct key.
   Replace with the real magic value obtained from the organisation. */
#define CRYPTO_MAGIC  0x00000000u

#endif /* SECRETS_H */
