#include "boot_manifest.h"
#include "boot_crc.h"
#include "usb_fs_service.h"
#include <string.h>
#include <stdio.h>

/* Maximum manifest file size.  A realistic manifest.ini is < 300 bytes. */
#define MANIFEST_BUF_SIZE  512U

/* Internal read buffer — keeps the allocation out of the caller. */
static uint8_t manifest_buf[MANIFEST_BUF_SIZE];

/* -----------------------------------------------------------------------
 * INI parser helpers
 * ----------------------------------------------------------------------- */

typedef enum
{
    SEC_NONE = 0,
    SEC_MANIFEST,
    SEC_APP_INT,
    SEC_APP_OSPI,
    SEC_INTEGRITY
} Section_t;

static const char *skip_line(const char *p, const char *end)
{
    while ((p < end) && (*p != '\n'))
    {
        p++;
    }
    if ((p < end) && (*p == '\n'))
    {
        p++;
    }
    return p;
}

static uint32_t line_len(const char *p, const char *end)
{
    const char *start = p;

    while ((p < end) && (*p != '\n') && (*p != '\r'))
    {
        p++;
    }
    return (uint32_t)(p - start);
}

static uint8_t starts_with(const char *line, uint32_t len, const char *prefix)
{
    uint32_t plen = (uint32_t)strlen(prefix);

    if (len < plen)
    {
        return 0U;
    }
    return (memcmp(line, prefix, plen) == 0) ? 1U : 0U;
}

static Section_t detect_section(const char *line, uint32_t len)
{
    if ((len >= 2U) && (line[0] == '['))
    {
        if (starts_with(line, len, "[manifest]"))    return SEC_MANIFEST;
        if (starts_with(line, len, "[app_int]"))     return SEC_APP_INT;
        if (starts_with(line, len, "[app_ospi]"))    return SEC_APP_OSPI;
        if (starts_with(line, len, "[integrity]"))   return SEC_INTEGRITY;
    }
    return SEC_NONE;
}

/* Return pointer to the value after "key=", or NULL if no match. */
static const char *match_key(const char *line, uint32_t len,
                             const char *key, uint32_t *val_len)
{
    uint32_t klen = (uint32_t)strlen(key);

    /* Need at least key + '=' */
    if (len <= klen)
    {
        return NULL;
    }
    if (memcmp(line, key, klen) != 0)
    {
        return NULL;
    }
    if (line[klen] != '=')
    {
        return NULL;
    }

    *val_len = len - klen - 1U;
    return &line[klen + 1U];
}

static void copy_str(const char *src, uint32_t src_len,
                     char *dst, uint32_t dst_cap)
{
    /* Trim trailing spaces/tabs */
    while ((src_len > 0U) && ((src[src_len - 1U] == ' ') || (src[src_len - 1U] == '\t')))
    {
        src_len--;
    }

    if (src_len >= dst_cap)
    {
        src_len = dst_cap - 1U;
    }

    strncpy(dst, src, src_len);
    dst[src_len] = '\0';
}

static uint8_t parse_uint32_dec(const char *s, uint32_t len, uint32_t *out)
{
    uint32_t val = 0U;
    uint32_t i;

    if (len == 0U)
    {
        return 0U;
    }
    for (i = 0U; i < len; i++)
    {
        uint8_t c = (uint8_t)s[i];

        if ((c < '0') || (c > '9'))
        {
            return 0U;
        }
        val = (val * 10U) + (uint32_t)(c - '0');
    }
    *out = val;
    return 1U;
}

static uint8_t parse_uint32_hex(const char *s, uint32_t len, uint32_t *out)
{
    uint32_t val = 0U;
    uint32_t i;

    if (len == 0U)
    {
        return 0U;
    }
    for (i = 0U; i < len; i++)
    {
        uint8_t c = (uint8_t)s[i];

        if ((c >= '0') && (c <= '9'))
        {
            val = (val << 4) | (uint32_t)(c - '0');
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            val = (val << 4) | (uint32_t)(c - 'A' + 10);
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            val = (val << 4) | (uint32_t)(c - 'a' + 10);
        }
        else
        {
            return 0U;
        }
    }
    *out = val;
    return 1U;
}

/* -----------------------------------------------------------------------
 * Parse a key=value line into the appropriate field of the manifest.
 * ----------------------------------------------------------------------- */
static void parse_kv(Section_t sec, const char *line, uint32_t len,
                     BootManifest_t *m, uint32_t *manifest_crc_out,
                     uint8_t *has_integrity)
{
    const char *val;
    uint32_t vlen;
    BootManifestImage_t *img = NULL;

    switch (sec)
    {
    case SEC_MANIFEST:
        if ((val = match_key(line, len, "product", &vlen)) != NULL)
            copy_str(val, vlen, m->product, sizeof(m->product));
        else if ((val = match_key(line, len, "hw_revision", &vlen)) != NULL)
            copy_str(val, vlen, m->hw_revision, sizeof(m->hw_revision));
        else if ((val = match_key(line, len, "sw_version", &vlen)) != NULL)
            copy_str(val, vlen, m->sw_version, sizeof(m->sw_version));
        else if ((val = match_key(line, len, "o3_lib_version", &vlen)) != NULL)
            copy_str(val, vlen, m->o3_lib_version, sizeof(m->o3_lib_version));
        else if ((val = match_key(line, len, "build_date", &vlen)) != NULL)
            copy_str(val, vlen, m->build_date, sizeof(m->build_date));
        break;

    case SEC_APP_INT:
        img = &m->app_int;
        break;

    case SEC_APP_OSPI:
        img = &m->app_ospi;
        break;

    case SEC_INTEGRITY:
        if ((val = match_key(line, len, "manifest_crc32", &vlen)) != NULL)
        {
            if (parse_uint32_hex(val, vlen, manifest_crc_out))
            {
                *has_integrity = 1U;
            }
        }
        break;

    default:
        break;
    }

    if (img != NULL)
    {
        if ((val = match_key(line, len, "filename", &vlen)) != NULL)
            copy_str(val, vlen, img->filename, sizeof(img->filename));
        else if ((val = match_key(line, len, "size", &vlen)) != NULL)
            parse_uint32_dec(val, vlen, &img->size);
        else if ((val = match_key(line, len, "crc32", &vlen)) != NULL)
            parse_uint32_hex(val, vlen, &img->crc32);
    }
}

/* -----------------------------------------------------------------------
 * Core parser — operates on a raw buffer, no I/O.
 * ----------------------------------------------------------------------- */
static BootManifestResult_t parse_buf(const uint8_t *buf, uint32_t len,
                                      BootManifest_t *out,
                                      uint32_t *integrity_offset,
                                      uint32_t *manifest_crc_stored,
                                      uint8_t  *has_integrity)
{
    const char *p   = (const char *)buf;
    const char *end = p + len;
    Section_t sec   = SEC_NONE;

    *integrity_offset   = len;
    *manifest_crc_stored = 0U;
    *has_integrity       = 0U;

    /* Skip UTF-8 BOM if present */
    if ((len >= 3U) &&
        ((uint8_t)p[0] == 0xEFU) &&
        ((uint8_t)p[1] == 0xBBU) &&
        ((uint8_t)p[2] == 0xBFU))
    {
        p += 3;
    }

    while (p < end)
    {
        uint32_t llen = line_len(p, end);

        /* Skip blank lines and comments */
        if ((llen == 0U) || (p[0] == '#') || (p[0] == ';'))
        {
            p = skip_line(p, end);
            continue;
        }

        /* Section header? */
        if (p[0] == '[')
        {
            Section_t new_sec = detect_section(p, llen);

            if (new_sec != SEC_NONE)
            {
                sec = new_sec;
                if (sec == SEC_INTEGRITY)
                {
                    *integrity_offset = (uint32_t)(p - (const char *)buf);
                }
            }
            p = skip_line(p, end);
            continue;
        }

        /* Key=value */
        parse_kv(sec, p, llen, out, manifest_crc_stored, has_integrity);
        p = skip_line(p, end);
    }

    /* Validate required fields */
    if (out->app_int.crc32 == 0U)
    {
        out->error_msg = "MANIFEST FAIL: INT CRC";
        return BOOT_MANIFEST_ERR_PARSE;
    }
    if (out->app_int.size == 0U)
    {
        out->error_msg = "MANIFEST FAIL: INT SIZE";
        return BOOT_MANIFEST_ERR_PARSE;
    }
    if (out->app_ospi.crc32 == 0U)
    {
        out->error_msg = "MANIFEST FAIL: OSPI CRC";
        return BOOT_MANIFEST_ERR_PARSE;
    }
    if (out->app_ospi.size == 0U)
    {
        out->error_msg = "MANIFEST FAIL: OSPI SIZE";
        return BOOT_MANIFEST_ERR_PARSE;
    }
    if (*has_integrity == 0U)
    {
        out->error_msg = "MANIFEST FAIL: INTEGRITY";
        return BOOT_MANIFEST_ERR_PARSE;
    }

    return BOOT_MANIFEST_OK;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

BootManifestResult_t BootManifest_LoadAndParse(const char *path,
                                               BootManifest_t *out)
{
    UsbFsResult_t usb_result;
    BootManifestResult_t parse_result;
    uint32_t bytes_read = 0U;
    uint32_t integrity_offset;
    uint32_t manifest_crc_stored;
    uint8_t  has_integrity;

    memset(out, 0, sizeof(*out));

    /* Read manifest file */
    usb_result = UsbFsService_ReadFile(path, manifest_buf, 0U,
                                       MANIFEST_BUF_SIZE, &bytes_read);
    if (usb_result != USB_FS_RESULT_OK)
    {
        printf("[MANIFEST] read FAILED result=%u\n", (unsigned int)usb_result);
        return BOOT_MANIFEST_ERR_READ;
    }

    if (bytes_read >= MANIFEST_BUF_SIZE)
    {
        printf("[MANIFEST] file too large (%lu bytes)\n",
               (unsigned long)bytes_read);
        return BOOT_MANIFEST_ERR_TOO_LARGE;
    }

    /* Parse */
    parse_result = parse_buf(manifest_buf, bytes_read, out,
                             &integrity_offset, &manifest_crc_stored,
                             &has_integrity);
    if (parse_result != BOOT_MANIFEST_OK)
    {
        printf("[MANIFEST] parse FAILED\n");
        return parse_result;
    }

    /* Validate integrity CRC if present */
    if (has_integrity != 0U)
    {
        uint32_t computed = BootCrc32_Compute(0U, manifest_buf,
                                              integrity_offset);
        if (computed != manifest_crc_stored)
        {
            printf("[MANIFEST] integrity CRC MISMATCH: "
                   "computed=%08lX stored=%08lX\n",
                   (unsigned long)computed,
                   (unsigned long)manifest_crc_stored);
            return BOOT_MANIFEST_ERR_INTEGRITY;
        }
    }

    return BOOT_MANIFEST_OK;
}

void BootManifest_Print(const BootManifest_t *m)
{
    printf("[MANIFEST] product=%s  hw_rev=%s\n",
           m->product, m->hw_revision);
    printf("[MANIFEST] sw_version=%s\n", m->sw_version);
    printf("[MANIFEST] o3_lib_version=%s\n", m->o3_lib_version);
    printf("[MANIFEST] build_date=%s\n", m->build_date);
    printf("[MANIFEST] app_int: file=%s size=%lu crc32=%08lX\n",
           m->app_int.filename,
           (unsigned long)m->app_int.size,
           (unsigned long)m->app_int.crc32);
    printf("[MANIFEST] app_ospi: file=%s size=%lu crc32=%08lX\n",
           m->app_ospi.filename,
           (unsigned long)m->app_ospi.size,
           (unsigned long)m->app_ospi.crc32);
}
