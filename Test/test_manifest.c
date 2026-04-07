/* Unit tests for boot_manifest.c parser — runs on PC with gcc.
 *
 * Build:  gcc -Wall -Wextra -I../Boot/Inc -o test_manifest test_manifest.c
 * Run:    ./test_manifest
 *
 * Includes boot_manifest.c directly to access static parse_buf(). */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Provide stubs before including the source under test */
#include "stubs.h"

/* Include source directly to access static parse_buf() */
#include "../Boot/Src/boot_manifest.c"

/* ----------------------------------------------------------------------- */

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  %-45s ", name); \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while (0)

#define FAIL(msg) \
    do { \
        printf("FAIL: %s\n", msg); \
    } while (0)

/* Helper: call parse_buf and return result */
static BootManifestResult_t run_parse(const char *text, BootManifest_t *m,
                                      uint32_t *integrity_off,
                                      uint32_t *stored_crc,
                                      uint8_t *has_int)
{
    memset(m, 0, sizeof(*m));
    return parse_buf((const uint8_t *)text, (uint32_t)strlen(text),
                     m, integrity_off, stored_crc, has_int);
}

/* ----------------------------------------------------------------------- */

static const char *VALID_MANIFEST =
    "[manifest]\n"
    "product=O3\n"
    "hw_revision=1\n"
    "sw_version=V1.R1.P1_b\n"
    "o3_lib_version=V0.R0.P0_a\n"
    "build_date=Apr 6 2026 11:26:51\n"
    "\n"
    "[app_int]\n"
    "filename=app_int.bin\n"
    "size=711856\n"
    "crc32=A456A479\n"
    "\n"
    "[app_ospi]\n"
    "filename=app_ospi.bin\n"
    "size=15319100\n"
    "crc32=6E3CCCFC\n"
    "\n"
    "[integrity]\n"
    "manifest_crc32=C9465C96\n";

/* ----------------------------------------------------------------------- */

static void test_valid_manifest(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;

    TEST("Valid manifest parses OK");
    BootManifestResult_t r = run_parse(VALID_MANIFEST, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    if (strcmp(m.product, "O3") != 0) { FAIL("product"); return; }
    if (m.hw_revision != 1U) { FAIL("hw_revision"); return; }
    if (strcmp(m.sw_version, "V1.R1.P1_b") != 0) { FAIL("sw_version"); return; }
    if (strcmp(m.o3_lib_version, "V0.R0.P0_a") != 0) { FAIL("o3_lib_version"); return; }
    if (strcmp(m.build_date, "Apr 6 2026 11:26:51") != 0) { FAIL("build_date"); return; }
    if (m.app_int.size != 711856U) { FAIL("app_int.size"); return; }
    if (m.app_int.crc32 != 0xA456A479U) { FAIL("app_int.crc32"); return; }
    if (strcmp(m.app_int.filename, "app_int.bin") != 0) { FAIL("app_int.filename"); return; }
    if (m.app_ospi.size != 15319100U) { FAIL("app_ospi.size"); return; }
    if (m.app_ospi.crc32 != 0x6E3CCCFCU) { FAIL("app_ospi.crc32"); return; }
    if (hint != 1U) { FAIL("has_integrity"); return; }
    if (scrc != 0xC9465C96U) { FAIL("manifest_crc32"); return; }
    PASS();
}

static void test_missing_app_int_crc32(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=711856\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n"
        "[integrity]\nmanifest_crc32=00000000\n";

    TEST("Missing [app_int] crc32");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_missing_app_int_size(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n"
        "[integrity]\nmanifest_crc32=00000000\n";

    TEST("Missing [app_int] size");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_missing_app_ospi_crc32(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=711856\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\n"
        "[integrity]\nmanifest_crc32=00000000\n";

    TEST("Missing [app_ospi] crc32");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_missing_app_ospi_size(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=711856\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\ncrc32=6E3CCCFC\n"
        "[integrity]\nmanifest_crc32=00000000\n";

    TEST("Missing [app_ospi] size");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_missing_integrity_section(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=711856\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n";

    TEST("Missing [integrity] section");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_missing_manifest_crc32_field(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=711856\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n"
        "[integrity]\n";

    TEST("Empty [integrity] section");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_invalid_hex_crc(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=711856\ncrc32=ZZZZZZZZ\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n"
        "[integrity]\nmanifest_crc32=00000000\n";

    TEST("Invalid hex in crc32");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_invalid_decimal_size(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[app_int]\nfilename=app_int.bin\nsize=abc\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n"
        "[integrity]\nmanifest_crc32=00000000\n";

    TEST("Invalid decimal in size");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_empty_file(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;

    TEST("Empty file");
    memset(&m, 0, sizeof(m));
    BootManifestResult_t r = parse_buf((const uint8_t *)"", 0U,
                                       &m, &ioff, &scrc, &hint);
    if (r == BOOT_MANIFEST_ERR_PARSE) { PASS(); } else { FAIL("expected ERR_PARSE"); }
}

static void test_windows_line_endings(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[manifest]\r\n"
        "product=O3\r\n"
        "hw_revision=1\r\n"
        "sw_version=V1.R1.P1_b\r\n"
        "o3_lib_version=V0.R0.P0_a\r\n"
        "build_date=Apr 6 2026 11:26:51\r\n"
        "\r\n"
        "[app_int]\r\n"
        "filename=app_int.bin\r\n"
        "size=711856\r\n"
        "crc32=A456A479\r\n"
        "\r\n"
        "[app_ospi]\r\n"
        "filename=app_ospi.bin\r\n"
        "size=15319100\r\n"
        "crc32=6E3CCCFC\r\n"
        "\r\n"
        "[integrity]\r\n"
        "manifest_crc32=C9465C96\r\n";

    TEST("Windows line endings (\\r\\n)");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    if (m.app_int.crc32 != 0xA456A479U) { FAIL("crc32 value"); return; }
    if (strcmp(m.product, "O3") != 0) { FAIL("product has trailing CR"); return; }
    PASS();
}

static void test_utf8_bom(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    /* BOM + valid manifest */
    char text[1024];
    text[0] = (char)0xEF;
    text[1] = (char)0xBB;
    text[2] = (char)0xBF;
    strcpy(&text[3],
        "[app_int]\nfilename=app_int.bin\nsize=711856\ncrc32=A456A479\n"
        "[app_ospi]\nfilename=app_ospi.bin\nsize=15319100\ncrc32=6E3CCCFC\n"
        "[integrity]\nmanifest_crc32=00000000\n");

    TEST("UTF-8 BOM at start");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    if (m.app_int.crc32 != 0xA456A479U) { FAIL("crc32"); return; }
    PASS();
}

static void test_comments_and_blank_lines(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "# This is a comment\n"
        "; Another comment\n"
        "\n"
        "[app_int]\n"
        "filename=app_int.bin\n"
        "size=711856\n"
        "crc32=A456A479\n"
        "\n"
        "# mid-file comment\n"
        "[app_ospi]\n"
        "filename=app_ospi.bin\n"
        "size=15319100\n"
        "crc32=6E3CCCFC\n"
        "\n"
        "[integrity]\n"
        "manifest_crc32=00000000\n";

    TEST("Comments and blank lines");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    PASS();
}

static void test_trailing_spaces(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[manifest]\n"
        "product=O3   \n"
        "sw_version=V1.R1.P1_b   \n"
        "[app_int]\n"
        "filename=app_int.bin\n"
        "size=711856\n"
        "crc32=A456A479\n"
        "[app_ospi]\n"
        "filename=app_ospi.bin\n"
        "size=15319100\n"
        "crc32=6E3CCCFC\n"
        "[integrity]\n"
        "manifest_crc32=00000000\n";

    TEST("Trailing spaces trimmed");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    if (strcmp(m.product, "O3") != 0) { FAIL("product has trailing spaces"); return; }
    if (strcmp(m.sw_version, "V1.R1.P1_b") != 0) { FAIL("sw_version has trailing spaces"); return; }
    PASS();
}

static void test_integrity_offset(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;

    TEST("Integrity offset points to [integrity]");
    BootManifestResult_t r = run_parse(VALID_MANIFEST, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    /* The offset should point to the '[' of [integrity] */
    if (memcmp(VALID_MANIFEST + ioff, "[integrity]", 11) != 0)
    {
        FAIL("offset does not point to [integrity]");
        return;
    }
    PASS();
}

static void test_unknown_section_ignored(void)
{
    BootManifest_t m;
    uint32_t ioff, scrc;
    uint8_t hint;
    const char *text =
        "[unknown]\n"
        "foo=bar\n"
        "[app_int]\n"
        "filename=app_int.bin\n"
        "size=711856\n"
        "crc32=A456A479\n"
        "[app_ospi]\n"
        "filename=app_ospi.bin\n"
        "size=15319100\n"
        "crc32=6E3CCCFC\n"
        "[integrity]\n"
        "manifest_crc32=00000000\n";

    TEST("Unknown section ignored");
    BootManifestResult_t r = run_parse(text, &m, &ioff, &scrc, &hint);
    if (r != BOOT_MANIFEST_OK) { FAIL("parse failed"); return; }
    PASS();
}

/* ----------------------------------------------------------------------- */

int main(void)
{
    printf("=== boot_manifest parser tests ===\n\n");

    test_valid_manifest();
    test_missing_app_int_crc32();
    test_missing_app_int_size();
    test_missing_app_ospi_crc32();
    test_missing_app_ospi_size();
    test_missing_integrity_section();
    test_missing_manifest_crc32_field();
    test_invalid_hex_crc();
    test_invalid_decimal_size();
    test_empty_file();
    test_windows_line_endings();
    test_utf8_bom();
    test_comments_and_blank_lines();
    test_trailing_spaces();
    test_integrity_offset();
    test_unknown_section_ignored();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
