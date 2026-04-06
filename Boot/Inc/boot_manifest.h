#ifndef BOOT_MANIFEST_H
#define BOOT_MANIFEST_H

#include <stdint.h>

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

typedef struct
{
    char     filename[32];
    uint32_t size;
    uint32_t crc32;
} BootManifestImage_t;

typedef struct
{
    char     product[16];
    uint32_t hw_revision;
    char     sw_version[20];
    char     o3_lib_version[20];
    char     build_date[32];

    BootManifestImage_t app_int;
    BootManifestImage_t app_ospi;
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

#ifdef __cplusplus
}
#endif

#endif /* BOOT_MANIFEST_H */
