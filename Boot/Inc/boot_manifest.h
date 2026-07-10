#ifndef BOOT_MANIFEST_H
#define BOOT_MANIFEST_H

#include <stdint.h>
#include "boot_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BOOT_MANIFEST_OK = 0,
    BOOT_MANIFEST_ERR_READ,
    BOOT_MANIFEST_ERR_TOO_LARGE,
    BOOT_MANIFEST_ERR_PARSE,
    BOOT_MANIFEST_ERR_INTEGRITY
} BootManifestResult_t;

typedef enum
{
    BOOT_SIG_OK = 0,
    BOOT_SIG_ERR_MISSING,    /* manifest.sig not found                    */
    BOOT_SIG_ERR_READ,       /* USB read error                            */
    BOOT_SIG_ERR_BAD_KEY     /* decryption failed with both known keys    */
} BootSigResult_t;

typedef struct
{
    char     filename[32];
    uint32_t size;
    uint32_t crc32;
} BootManifestImage_t;

typedef struct
{
    char     product[20];
    char     hw_revision[20];
    char     sw_version[20];
    char     o3_lib_version[20];
    char     build_date[32];

    BootManifestImage_t app_int;
    BootManifestImage_t app_ospi;

    const char *error_msg;  /* human-readable error detail (NULL when OK) */
} BootManifest_t;

/**
 * @brief  Read manifest.ini from USB, parse it and validate its integrity CRC.
 * @param  path  FatFs path to the manifest file (e.g. "0:/UPDATE/manifest.ini")
 * @param  out   Parsed manifest (zeroed then populated)
 * @retval BOOT_MANIFEST_OK on success
 */
BootManifestResult_t BootManifest_LoadAndParse(const char *path,
                                               BootManifest_t *out);

/**
 * @brief  Print parsed manifest fields to SWV for diagnostics.
 */
void BootManifest_Print(const BootManifest_t *m);

/**
 * @brief  Load and decrypt manifest.sig (companion of manifest.ini). Only
 *         validates the key/signature — does NOT check sizes or SHA-256 of
 *         app_int.bin/app_ospi.bin (that happens in the BOOT_PRE_FLASH_CRC_CHECK
 *         loop in main.c, see Plan_Cifrado_Bootloader_Consola.txt).
 * @param  sig_path  FatFs path to manifest.sig (e.g. "0:/UPDATE/manifest.sig")
 * @param  out       Decrypted payload (sizes + expected SHA-256 of both binaries)
 * @retval BOOT_SIG_OK on success
 */
BootSigResult_t BootSig_LoadAndDecrypt(const char *sig_path,
                                       boot_sig_payload_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_MANIFEST_H */
