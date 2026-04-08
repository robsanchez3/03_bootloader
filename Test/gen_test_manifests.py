#!/usr/bin/env python3
"""Generate test manifest files for each bootloader error scenario.

Each file is a valid INI with one specific defect. The manifest CRC in
[integrity] is recalculated so the only error is the intended one —
except for tests that specifically target CRC integrity.

Output directory: Test/manifests/
Copy the desired manifest.ini to the USB drive's UPDATE/ folder to test.

Usage:  python3 gen_test_manifests.py
"""

import os
import struct

# -------------------------------------------------------------------
# CRC32 matching the STM32 HW CRC config:
#   polynomial = 0x04C11DB7 (default)
#   init = 0xFFFFFFFF
#   input byte inversion, output inversion, final XOR 0xFFFFFFFF
# This is standard CRC32 (IEEE 802.3), same as binascii.crc32.
# -------------------------------------------------------------------
import binascii

def crc32(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF


# -------------------------------------------------------------------
# Reference values from the real manifest
# -------------------------------------------------------------------
PRODUCT        = "O3 interface"
HW_REVISION    = "edt EVK070027B"
SW_VERSION     = "V1.R1.P1_b"
O3_LIB_VERSION = "V0.R0.P0_a"
BUILD_DATE     = "Apr 6 2026 11:26:51"

APP_INT_FILENAME = "app_int.bin"
APP_INT_SIZE     = 711856
APP_INT_CRC32    = 0xA456A479

APP_OSPI_FILENAME = "app_ospi.bin"
APP_OSPI_SIZE     = 15319100
APP_OSPI_CRC32    = 0x6E3CCCFC


def build_manifest(*, product=PRODUCT, hw_revision=HW_REVISION,
                   sw_version=SW_VERSION, o3_lib_version=O3_LIB_VERSION,
                   build_date=BUILD_DATE,
                   int_filename=APP_INT_FILENAME, int_size=APP_INT_SIZE,
                   int_crc32=APP_INT_CRC32,
                   ospi_filename=APP_OSPI_FILENAME, ospi_size=APP_OSPI_SIZE,
                   ospi_crc32=APP_OSPI_CRC32,
                   include_integrity=True, override_manifest_crc=None,
                   omit_field=None):
    """Build a manifest.ini string with optional defects."""

    lines = []

    # [manifest]
    lines.append("[manifest]")
    if omit_field != "product":
        lines.append(f"product={product}")
    if omit_field != "hw_revision":
        lines.append(f"hw_revision={hw_revision}")
    if omit_field != "sw_version":
        lines.append(f"sw_version={sw_version}")
    if omit_field != "o3_lib_version":
        lines.append(f"o3_lib_version={o3_lib_version}")
    if omit_field != "build_date":
        lines.append(f"build_date={build_date}")
    lines.append("")

    # [app_int]
    lines.append("[app_int]")
    if omit_field != "int_filename":
        lines.append(f"filename={int_filename}")
    if omit_field != "int_size":
        lines.append(f"size={int_size}")
    if omit_field != "int_crc32":
        lines.append(f"crc32={int_crc32:08X}")
    lines.append("")

    # [app_ospi]
    lines.append("[app_ospi]")
    if omit_field != "ospi_filename":
        lines.append(f"filename={ospi_filename}")
    if omit_field != "ospi_size":
        lines.append(f"size={ospi_size}")
    if omit_field != "ospi_crc32":
        lines.append(f"crc32={ospi_crc32:08X}")
    lines.append("")

    # [integrity]
    if include_integrity:
        body = "\n".join(lines) + "\n"
        body_bytes = body.encode("ascii")
        manifest_crc = crc32(body_bytes)

        if override_manifest_crc is not None:
            manifest_crc = override_manifest_crc

        lines.append("[integrity]")
        lines.append(f"manifest_crc32={manifest_crc:08X}")

    return "\n".join(lines) + "\n"


def write_manifest(outdir, name, description, content):
    """Write a manifest file and print info."""
    filepath = os.path.join(outdir, name)
    with open(filepath, "w", newline="\n") as f:
        f.write(content)
    print(f"  {name:40s} -> {description}")


def main():
    outdir = os.path.join(os.path.dirname(__file__), "manifests")
    os.makedirs(outdir, exist_ok=True)

    print("Generating test manifests:\n")

    # 0. Valid reference manifest
    write_manifest(outdir, "00_valid.ini",
                   "Valid manifest (should pass all checks)",
                   build_manifest())

    # --- MANIFEST PARSE FAILED cases ---

    # 1. Missing [app_int] crc32
    write_manifest(outdir, "01_no_int_crc.ini",
                   "MANIFEST PARSE FAILED (missing app_int crc32)",
                   build_manifest(omit_field="int_crc32"))

    # 2. Missing [app_int] size
    write_manifest(outdir, "02_no_int_size.ini",
                   "MANIFEST PARSE FAILED (missing app_int size)",
                   build_manifest(omit_field="int_size"))

    # 3. Missing [app_ospi] crc32
    write_manifest(outdir, "03_no_ospi_crc.ini",
                   "MANIFEST PARSE FAILED (missing app_ospi crc32)",
                   build_manifest(omit_field="ospi_crc32"))

    # 4. Missing [app_ospi] size
    write_manifest(outdir, "04_no_ospi_size.ini",
                   "MANIFEST PARSE FAILED (missing app_ospi size)",
                   build_manifest(omit_field="ospi_size"))

    # 5. Missing [integrity] section
    write_manifest(outdir, "05_no_integrity.ini",
                   "MANIFEST PARSE FAILED (no [integrity] section)",
                   build_manifest(include_integrity=False))

    # --- WRONG PRODUCT / HW REVISION ---

    # 6. Wrong product
    write_manifest(outdir, "06_wrong_product.ini",
                   "MANIFEST FAIL: PRODUCT (product=O3 other)",
                   build_manifest(product="O3 other"))

    # 7. Wrong hw_revision
    write_manifest(outdir, "07_wrong_hw_rev.ini",
                   "MANIFEST FAIL: HW (hw_revision=edt OTHER)",
                   build_manifest(hw_revision="edt OTHER"))

    # --- MANIFEST CRC FAILED ---

    # 8. Wrong manifest CRC
    write_manifest(outdir, "08_bad_manifest_crc.ini",
                   "MANIFEST CRC FAILED (tampered CRC)",
                   build_manifest(override_manifest_crc=0xDEADBEEF))

    # --- INT FILE SIZE MISMATCH ---

    # 9. Wrong app_int size (manifest says different than actual .bin)
    write_manifest(outdir, "09_wrong_int_size.ini",
                   "INT FILE SIZE MISMATCH (size=999999)",
                   build_manifest(int_size=999999))

    # --- OSPI FILE SIZE MISMATCH ---

    # 10. Wrong app_ospi size
    write_manifest(outdir, "10_wrong_ospi_size.ini",
                   "OSPI FILE SIZE MISMATCH (size=999999)",
                   build_manifest(ospi_size=999999))

    # --- INT BIN CRC MISMATCH ---

    # 11. Wrong app_int CRC (manifest CRC valid, but bin CRC won't match)
    write_manifest(outdir, "11_wrong_int_crc.ini",
                   "INT BIN CRC MISMATCH (crc32=DEADBEEF)",
                   build_manifest(int_crc32=0xDEADBEEF))

    # --- OSPI BIN CRC MISMATCH ---

    # 12. Wrong app_ospi CRC
    write_manifest(outdir, "12_wrong_ospi_crc.ini",
                   "OSPI BIN CRC MISMATCH (crc32=DEADBEEF)",
                   build_manifest(ospi_crc32=0xDEADBEEF))

    print(f"\nGenerated {13} files in {outdir}/")
    print("\nUsage: copy the desired file as 'manifest.ini' to USB UPDATE/ folder.")
    print("Remember: FatFs uses 8.3 names, so the file must be named 'manifest.ini'.")


if __name__ == "__main__":
    main()
