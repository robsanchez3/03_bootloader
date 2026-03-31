#include "boot_flash.h"
#include "boot_jump.h"
#include "stm32u5xx_hal.h"
#include <string.h>
#include <stdio.h>

#define FLASH_PAGE_SZ       0x2000U          /* 8 KB */
#define FLASH_QUADWORD_SZ   16U              /* 128 bits = programming unit */
#define FLASH_BANK1_BASE    0x08000000U
#define FLASH_BANK2_BASE    0x08100000U

/* Convert an absolute flash address to bank + page number. */
static BootFlashResult_t AddressToPage(uint32_t address, uint32_t *bank, uint32_t *page)
{
    if (address >= FLASH_BANK2_BASE)
    {
        *bank = FLASH_BANK_2;
        *page = (address - FLASH_BANK2_BASE) / FLASH_PAGE_SZ;
    }
    else if (address >= FLASH_BANK1_BASE)
    {
        *bank = FLASH_BANK_1;
        *page = (address - FLASH_BANK1_BASE) / FLASH_PAGE_SZ;
    }
    else
    {
        return BOOT_FLASH_ERR_RANGE;
    }

    return BOOT_FLASH_OK;
}

BootFlashResult_t BootFlash_EraseAppArea(uint32_t size)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0U;
    uint32_t start_addr = APP_BASE;
    uint32_t end_addr   = start_addr + size;
    uint32_t bank;
    uint32_t first_page;
    uint32_t nb_pages;
    HAL_StatusTypeDef hal_status;

    /* Clamp to app area */
    if (end_addr > (APP_END + 1U))
    {
        end_addr = APP_END + 1U;
    }

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return BOOT_FLASH_ERR_UNLOCK;
    }

    /* Erase Bank 1 portion (APP_BASE to end of Bank 1) */
    if (start_addr < FLASH_BANK2_BASE)
    {
        uint32_t bank1_end = (end_addr < FLASH_BANK2_BASE) ? end_addr : FLASH_BANK2_BASE;

        (void)AddressToPage(start_addr, &bank, &first_page);
        nb_pages = (bank1_end - start_addr + FLASH_PAGE_SZ - 1U) / FLASH_PAGE_SZ;

        printf("[FLASH] erase bank1 page %lu count %lu\n",
               (unsigned long)first_page, (unsigned long)nb_pages);

        erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
        erase_init.Banks     = FLASH_BANK_1;
        erase_init.Page      = first_page;
        erase_init.NbPages   = nb_pages;

        hal_status = HAL_FLASHEx_Erase(&erase_init, &page_error);
        if (hal_status != HAL_OK)
        {
            printf("[FLASH] erase bank1 FAILED page_error=%lu HAL=%u\n",
                   (unsigned long)page_error, (unsigned int)hal_status);
            (void)HAL_FLASH_Lock();
            return BOOT_FLASH_ERR_ERASE;
        }
    }

    /* Erase Bank 2 portion if needed */
    if (end_addr > FLASH_BANK2_BASE)
    {
        uint32_t bank2_start = (start_addr > FLASH_BANK2_BASE) ? start_addr : FLASH_BANK2_BASE;

        (void)AddressToPage(bank2_start, &bank, &first_page);
        nb_pages = (end_addr - bank2_start + FLASH_PAGE_SZ - 1U) / FLASH_PAGE_SZ;

        printf("[FLASH] erase bank2 page %lu count %lu\n",
               (unsigned long)first_page, (unsigned long)nb_pages);

        erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
        erase_init.Banks     = FLASH_BANK_2;
        erase_init.Page      = first_page;
        erase_init.NbPages   = nb_pages;

        hal_status = HAL_FLASHEx_Erase(&erase_init, &page_error);
        if (hal_status != HAL_OK)
        {
            printf("[FLASH] erase bank2 FAILED page_error=%lu HAL=%u\n",
                   (unsigned long)page_error, (unsigned int)hal_status);
            (void)HAL_FLASH_Lock();
            return BOOT_FLASH_ERR_ERASE;
        }
    }

    (void)HAL_FLASH_Lock();

    printf("[FLASH] erase OK %lu bytes\n", (unsigned long)(end_addr - start_addr));
    return BOOT_FLASH_OK;
}

BootFlashResult_t BootFlash_Program(uint32_t address, const uint8_t *data, uint32_t len)
{
    uint32_t offset = 0U;
    uint32_t quadword[4];
    HAL_StatusTypeDef hal_status;

    if ((address < APP_BASE) || ((address + len) > (APP_END + 1U)))
    {
        return BOOT_FLASH_ERR_RANGE;
    }

    if ((address & (FLASH_QUADWORD_SZ - 1U)) != 0U)
    {
        return BOOT_FLASH_ERR_ALIGNMENT;
    }

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return BOOT_FLASH_ERR_UNLOCK;
    }

    while (offset < len)
    {
        uint32_t remaining = len - offset;
        uint32_t chunk = (remaining >= FLASH_QUADWORD_SZ) ? FLASH_QUADWORD_SZ : remaining;

        /* Prepare quadword — pad with 0xFF if last chunk is partial */
        memset(quadword, 0xFF, FLASH_QUADWORD_SZ);
        memcpy(quadword, &data[offset], chunk);

        hal_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
                                       address + offset,
                                       (uint32_t)quadword);
        if (hal_status != HAL_OK)
        {
            printf("[FLASH] program FAILED at 0x%08lX HAL=%u flash_err=0x%08lX\n",
                   (unsigned long)(address + offset),
                   (unsigned int)hal_status,
                   (unsigned long)HAL_FLASH_GetError());
            (void)HAL_FLASH_Lock();
            return BOOT_FLASH_ERR_PROGRAM;
        }

        offset += FLASH_QUADWORD_SZ;
    }

    (void)HAL_FLASH_Lock();
    return BOOT_FLASH_OK;
}

BootFlashResult_t BootFlash_Verify(uint32_t address, const uint8_t *data, uint32_t len)
{
    const uint8_t *flash_ptr = (const uint8_t *)address;

    if (memcmp(flash_ptr, data, len) != 0)
    {
        /* Find first mismatch for diagnostics */
        uint32_t i;
        for (i = 0U; i < len; i++)
        {
            if (flash_ptr[i] != data[i])
            {
                printf("[FLASH] verify MISMATCH at offset %lu: flash=0x%02X data=0x%02X\n",
                       (unsigned long)i,
                       (unsigned int)flash_ptr[i],
                       (unsigned int)data[i]);
                break;
            }
        }
        return BOOT_FLASH_ERR_VERIFY;
    }

    return BOOT_FLASH_OK;
}
