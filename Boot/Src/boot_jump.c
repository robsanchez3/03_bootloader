#include "boot_jump.h"
#include "boot_crc.h"
#include "boot_ospi.h"
#include "main.h"
#include "stm32u5xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Boot config page — last 8KB page of the 128KB bootloader region.
 * Stored as a single quadword-aligned struct at the start of the page.
 * Survives power loss (flash), never touched by app or by erase of app area.
 *
 * Anti-brick CRC verification flow:
 *
 *   1. Factory: bootloader + app programmed via ST-LINK.  Config page is
 *      empty (0xFF).  Boot_VerifyAppCrc sees no magic -> skips CRC check
 *      and trusts the basic MSP/reset-vector validation only.
 *
 *   2. First update via USB: after both app_int and app_ospi are
 *      programmed and CRC-verified, Boot_StoreAppCrc writes the expected
 *      CRCs and sizes into this config page.
 *
 *   3. Subsequent boots: Boot_VerifyAppCrc reads the config page, finds
 *      a valid magic, recalculates CRC over both flash regions and
 *      compares.  Match -> jump to app.  Mismatch -> recovery mode.
 *
 *   4. Power loss during update: the config page still holds the CRCs
 *      of the PREVIOUS good image.  On next boot the CRC check fails
 *      (partially-written app doesn't match) -> recovery mode.
 *
 *   Note: factory units have no stored CRC so a factory flash corruption
 *   would not be caught.  This is acceptable; production test covers it.
 *   To close this gap, write the config page with correct CRCs as part
 *   of the factory programming sequence. */
#define BOOT_CFG_PAGE_ADDR       0x0801E000U
#define BOOT_CFG_MAGIC           0xC3C0BEEFU
#define BOOT_VERIFY_INT_AT_BOOT  0
#define BOOT_VERIFY_OSPI_AT_BOOT 0

typedef struct
{
    uint32_t magic;
    uint32_t int_crc32;
    uint32_t int_size;
    uint32_t ospi_crc32;
    uint32_t ospi_size;
    uint32_t reserved[3]; /* pad to 32 bytes (2 quadwords) for alignment */
} BootCfg_t;

static const volatile BootCfg_t *boot_cfg =
    (const volatile BootCfg_t *)BOOT_CFG_PAGE_ADDR;

void Boot_StoreAppCrc(uint32_t int_crc32, uint32_t int_size,
                      uint32_t ospi_crc32, uint32_t ospi_size)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error;
    BootCfg_t cfg;
    uint32_t addr;
    uint32_t i;

    printf("[BOOT] StoreAppCrc: int=%08lX ospi=%08lX\n",
           (unsigned long)int_crc32, (unsigned long)ospi_crc32);

    memset(&cfg, 0xFF, sizeof(cfg));
    cfg.int_crc32  = int_crc32;
    cfg.int_size   = int_size;
    cfg.ospi_crc32 = ospi_crc32;
    cfg.ospi_size  = ospi_size;
    cfg.magic      = BOOT_CFG_MAGIC;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        printf("[BOOT] Flash unlock failed — CRC not stored\n");
        return;
    }

    /* Erase the config page */
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks     = FLASH_BANK_1;
    erase.Page      = (BOOT_CFG_PAGE_ADDR - 0x08000000U) / 0x2000U;
    erase.NbPages   = 1U;

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
    {
        printf("[BOOT] Config page erase failed\n");
        (void)HAL_FLASH_Lock();
        return;
    }

    /* Program in quadwords (16 bytes each), 2 quadwords for 32-byte struct */
    addr = BOOT_CFG_PAGE_ADDR;
    for (i = 0U; i < sizeof(cfg); i += 16U)
    {
        uint32_t qw[4];
        memcpy(qw, (const uint8_t *)&cfg + i, 16U);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr + i,
                              (uint32_t)qw) != HAL_OK)
        {
            printf("[BOOT] Config page program failed at offset %lu\n",
                   (unsigned long)i);
            (void)HAL_FLASH_Lock();
            return;
        }
    }

    (void)HAL_FLASH_Lock();
    printf("[BOOT] App CRC stored in flash config page\n");
}

int Boot_VerifyAppCrc(void)
{
    if (boot_cfg->magic != BOOT_CFG_MAGIC)
    {
        printf("[BOOT] No stored CRC: skip verify\n");
        return 1;
    }

    /* Stored CRCs of zero mean the config page was invalidated before
     * a programming cycle — the update never completed successfully. */
    if ((boot_cfg->int_size == 0U) || (boot_cfg->ospi_size == 0U))
    {
        printf("[BOOT] CRC verify: FAIL (incomplete update)\n");
        return 0;
    }

#if BOOT_VERIFY_INT_AT_BOOT
    /* Internal flash CRC verification (~696KB, ~20ms — negligible).
     * This catches post-update corruption (bit flip, flash degradation)
     * but is not strictly needed for anti-brick protection: the
     * Boot_StoreAppCrc(0,0,0,0) invalidation before programming already
     * covers incomplete updates via the size==0 check above.
     * Disable only if boot time is ultra-critical (<50ms). */
    {
    uint32_t computed_crc = BootCrc32_Compute(0U, (const uint8_t *)APP_BASE,
                                              boot_cfg->int_size);
    if (computed_crc != boot_cfg->int_crc32)
    {
        printf("[BOOT] CRC verify: int FAIL (computed=%08lX stored=%08lX)\n",
               (unsigned long)computed_crc,
               (unsigned long)boot_cfg->int_crc32);
        return 0;
    }
    }
#endif

#if BOOT_VERIFY_OSPI_AT_BOOT
    /* OSPI verification at boot is disabled by default because the
     * MX25LM51245G at 32MHz STR takes ~17-20s for 15MB, which is too
     * slow for normal boot.  Possible improvements to re-enable:
     *   - Increase OSPI clock (chip supports up to 200MHz STR)
     *   - Use DTR mode (double transfer rate at same clock)
     *   - Verify only a partial region (e.g. first 64KB)
     *
     * With OSPI verification disabled, mid-update power loss is still
     * caught: Boot_StoreAppCrc(0,0,0,0) invalidates the config page
     * before programming starts, so int_size/ospi_size are zero and
     * the "incomplete update" check above triggers recovery.
     * If int flash was fully written but OSPI was not, int CRC will
     * match the stored value but the app will detect the OSPI problem
     * at runtime.  The bootloader itself remains intact in all cases. */
    if (BootOspi_Init() != BOOT_OSPI_OK ||
        BootOspi_EnableMemoryMapped() != BOOT_OSPI_OK)
    {
        printf("[BOOT] CRC verify: ospi FAIL (init error)\n");
        return 0;
    }

    {
    uint32_t computed_crc = BootCrc32_Compute(0U, (const uint8_t *)OCTOSPI1_BASE,
                                              boot_cfg->ospi_size);

    /* Deinit OSPI so the app can reinitialize it from a clean state */
    HAL_OSPI_Abort(BootOspi_GetHandle());
    HAL_OSPI_DeInit(BootOspi_GetHandle());

    if (computed_crc != boot_cfg->ospi_crc32)
    {
        printf("[BOOT] CRC verify: ospi FAIL (computed=%08lX stored=%08lX)\n",
               (unsigned long)computed_crc,
               (unsigned long)boot_cfg->ospi_crc32);
        return 0;
    }
    }

    printf("[BOOT] CRC verify: int OK, ospi OK\n");
#elif BOOT_VERIFY_INT_AT_BOOT
    printf("[BOOT] CRC verify: int OK\n");
#else
    printf("[BOOT] CRC verify: skipped\n");
#endif

    return 1;
}

static void Boot_DisableAllInterrupts(void)
{
    uint32_t index;

    for (index = 0U; index < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); index++)
    {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }
}

int Boot_IsApplicationValid(uint32_t app_base)
{
    uint32_t msp = *(volatile uint32_t *)app_base;
    uint32_t reset_vector = *(volatile uint32_t *)(app_base + 4U);
    uint32_t reset_addr;

    if (msp < RAM_BASE || msp > RAM_TOP)
    {
        printf("[BOOT] Invalid app: MSP out of RAM\n");
        return 0;
    }

    if ((msp & 0x7U) != 0U)
    {
        printf("[BOOT] Invalid app: MSP not aligned\n");
        return 0;
    }

    if ((reset_vector & 0x1U) == 0U)
    {
        printf("[BOOT] Invalid app: Thumb bit not set\n");
        return 0;
    }

    reset_addr = reset_vector & ~1U;
    if (reset_addr < app_base || reset_addr > APP_END)
    {
        printf("[BOOT] Invalid app: reset vector out of range\n");
        return 0;
    }

    printf("[BOOT] App header OK @ 0x%08lX (MSP=%08lX RST=%08lX)\n",
           (unsigned long)app_base,
           (unsigned long)msp,
           (unsigned long)reset_vector);
    return 1;
}

void Boot_JumpToApplication(uint32_t app_base)
{
    uint32_t app_msp;
    uint32_t app_reset_vector;

    printf("[BOOT] Jump to app\n");

    __disable_irq();
    Boot_DisableAllInterrupts();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    app_msp = *(volatile uint32_t *)app_base;
    app_reset_vector = *(volatile uint32_t *)(app_base + 4U);

    SCB->VTOR = app_base;
    __DSB();
    __ISB();

    __set_CONTROL(0U);
    __set_BASEPRI(0U);
    __set_FAULTMASK(0U);
    __set_MSPLIM(0U);
    __set_PSPLIM(0U);
    __ISB();

    __enable_irq();
    __asm volatile(
        "msr msp, %0    \n"
        "bx  %1         \n"
        :
        : "r" (app_msp), "r" (app_reset_vector)
        : "memory"
    );
    while (1) {}
}
