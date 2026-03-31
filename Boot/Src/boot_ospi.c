#include "boot_ospi.h"
#include "mx25lm51245g.h"
#include "stm32u5xx_hal.h"
#include <stdio.h>
#include <string.h>

#define OSPI_PAGE_SIZE       MX25LM51245G_PAGE_SIZE      /* 256 bytes */
#define OSPI_SECTOR_SIZE     MX25LM51245G_SECTOR_64K     /* 64 KB     */

static OSPI_HandleTypeDef hospi1;

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

static void BootOspi_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* OSPI clock from PLL1 (64 MHz, prescaler 2 → 32 MHz) */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection   = RCC_OSPICLKSOURCE_PLL1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        return;
    }

    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PF10 → CLK (AF3) */
    GPIO_InitStruct.Pin       = GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_OCTOSPI1;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* PF6, PF7, PF8, PF9 → IO0-IO3 (AF10) */
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* PC1, PC2, PC3 → IO4-IO6 (AF10) */
    GPIO_InitStruct.Pin       = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PC0 → IO7 (AF3) */
    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    GPIO_InitStruct.Alternate = GPIO_AF3_OCTOSPI1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PA1 → DQS, PA2 → NCS (AF10) */
    GPIO_InitStruct.Pin       = GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static int32_t BootOspi_MemoryReset(void)
{
    /* Reset in all possible modes to ensure clean state */
    if (MX25LM51245G_ResetEnable(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_ResetMemory(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_ResetEnable(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_ResetMemory(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_ResetEnable(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_DTR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_ResetMemory(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_DTR_TRANSFER) != MX25LM51245G_OK)
        return -1;

    HAL_Delay(MX25LM51245G_RESET_MAX_TIME);
    return 0;
}

static int32_t BootOspi_EnterOpiMode(void)
{
    uint8_t reg[2];

    /* Set dummy cycles to 6 */
    if (MX25LM51245G_WriteEnable(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_WriteCfg2Register(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER,
                                        MX25LM51245G_CR2_REG3_ADDR, MX25LM51245G_CR2_DC_6_CYCLES) != MX25LM51245G_OK)
        return -1;

    /* Enter SOPI (Octal STR) mode */
    if (MX25LM51245G_WriteEnable(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_WriteCfg2Register(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER,
                                        MX25LM51245G_CR2_REG1_ADDR, MX25LM51245G_CR2_SOPI) != MX25LM51245G_OK)
        return -1;

    HAL_Delay(MX25LM51245G_WRITE_REG_MAX_TIME);

    /* Verify OPI mode active */
    if (MX25LM51245G_AutoPollingMemReady(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        return -1;
    if (MX25LM51245G_ReadCfg2Register(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER,
                                       MX25LM51245G_CR2_REG1_ADDR, reg) != MX25LM51245G_OK)
        return -1;
    if (reg[0] != MX25LM51245G_CR2_SOPI)
        return -1;

    return 0;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

BootOspiResult_t BootOspi_Init(void)
{
    OSPIM_CfgTypeDef sOspiManagerCfg = {0};

    printf("[OSPI] init\n");

    BootOspi_MspInit();

    hospi1.Instance                  = OCTOSPI1;
    hospi1.Init.FifoThreshold        = 4;
    hospi1.Init.DualQuad             = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1.Init.MemoryType           = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi1.Init.DeviceSize           = 32;
    hospi1.Init.ChipSelectHighTime   = 2;
    hospi1.Init.FreeRunningClock     = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1.Init.ClockMode            = HAL_OSPI_CLOCK_MODE_0;
    hospi1.Init.WrapSize             = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1.Init.ClockPrescaler       = 2;
    hospi1.Init.SampleShifting       = HAL_OSPI_SAMPLE_SHIFTING_NONE;
    hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
    hospi1.Init.ChipSelectBoundary   = 0;
    hospi1.Init.DelayBlockBypass     = HAL_OSPI_DELAY_BLOCK_BYPASSED;
    hospi1.Init.MaxTran              = 0;
    hospi1.Init.Refresh              = 0;

    if (HAL_OSPI_Init(&hospi1) != HAL_OK)
    {
        printf("[OSPI] HAL_OSPI_Init FAILED\n");
        return BOOT_OSPI_ERR_INIT;
    }

    sOspiManagerCfg.ClkPort    = 1;
    sOspiManagerCfg.DQSPort    = 1;
    sOspiManagerCfg.NCSPort    = 1;
    sOspiManagerCfg.IOLowPort  = HAL_OSPIM_IOPORT_1_LOW;
    sOspiManagerCfg.IOHighPort = HAL_OSPIM_IOPORT_1_HIGH;

    if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        printf("[OSPI] HAL_OSPIM_Config FAILED\n");
        return BOOT_OSPI_ERR_INIT;
    }

    if (BootOspi_MemoryReset() != 0)
    {
        printf("[OSPI] memory reset FAILED\n");
        return BOOT_OSPI_ERR_INIT;
    }

    if (MX25LM51245G_AutoPollingMemReady(&hospi1, MX25LM51245G_SPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
    {
        printf("[OSPI] memory not ready after reset\n");
        return BOOT_OSPI_ERR_INIT;
    }

    if (BootOspi_EnterOpiMode() != 0)
    {
        printf("[OSPI] enter OPI mode FAILED\n");
        return BOOT_OSPI_ERR_INIT;
    }

    printf("[OSPI] init OK (OPI STR mode, 32 MHz)\n");
    return BOOT_OSPI_OK;
}

BootOspiResult_t BootOspi_Erase(uint32_t size)
{
    uint32_t address = 0U;
    uint32_t sector  = 0U;
    uint32_t total_sectors = (size + OSPI_SECTOR_SIZE - 1U) / OSPI_SECTOR_SIZE;

    printf("[OSPI] erase %lu sectors (%lu KB)\n",
           (unsigned long)total_sectors,
           (unsigned long)(total_sectors * 64U));

    while (address < size)
    {
        if (MX25LM51245G_WriteEnable(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        {
            printf("[OSPI] WriteEnable FAILED at sector %lu\n", (unsigned long)sector);
            return BOOT_OSPI_ERR_ERASE;
        }

        if (MX25LM51245G_BlockErase(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER,
                                     MX25LM51245G_4BYTES_SIZE, address, MX25LM51245G_ERASE_64K) != MX25LM51245G_OK)
        {
            printf("[OSPI] erase FAILED at address 0x%08lX\n", (unsigned long)address);
            return BOOT_OSPI_ERR_ERASE;
        }

        if (MX25LM51245G_AutoPollingMemReady(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        {
            printf("[OSPI] poll ready FAILED after erase at 0x%08lX\n", (unsigned long)address);
            return BOOT_OSPI_ERR_ERASE;
        }

        address += OSPI_SECTOR_SIZE;
        sector++;

        if ((sector & 15U) == 0U)
        {
            printf("[OSPI] erased %lu / %lu sectors\n",
                   (unsigned long)sector, (unsigned long)total_sectors);
        }
    }

    printf("[OSPI] erase OK\n");
    return BOOT_OSPI_OK;
}

BootOspiResult_t BootOspi_Program(uint32_t address, const uint8_t *data, uint32_t len)
{
    uint32_t offset = 0U;
    uint32_t chunk;

    while (offset < len)
    {
        chunk = len - offset;
        if (chunk > OSPI_PAGE_SIZE)
        {
            chunk = OSPI_PAGE_SIZE;
        }

        if (MX25LM51245G_WriteEnable(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        {
            printf("[OSPI] WriteEnable FAILED at 0x%08lX\n", (unsigned long)(address + offset));
            return BOOT_OSPI_ERR_PROGRAM;
        }

        if (MX25LM51245G_PageProgram(&hospi1, MX25LM51245G_OPI_MODE,
                                      MX25LM51245G_4BYTES_SIZE,
                                      (uint8_t *)&data[offset], address + offset, chunk) != MX25LM51245G_OK)
        {
            printf("[OSPI] program FAILED at 0x%08lX\n", (unsigned long)(address + offset));
            return BOOT_OSPI_ERR_PROGRAM;
        }

        if (MX25LM51245G_AutoPollingMemReady(&hospi1, MX25LM51245G_OPI_MODE, MX25LM51245G_STR_TRANSFER) != MX25LM51245G_OK)
        {
            printf("[OSPI] poll ready FAILED after program at 0x%08lX\n", (unsigned long)(address + offset));
            return BOOT_OSPI_ERR_PROGRAM;
        }

        offset += chunk;
    }

    return BOOT_OSPI_OK;
}

BootOspiResult_t BootOspi_EnableMemoryMapped(void)
{
    if (MX25LM51245G_EnableMemoryMappedModeSTR(&hospi1, MX25LM51245G_OPI_MODE,
                                                MX25LM51245G_4BYTES_SIZE) != MX25LM51245G_OK)
    {
        printf("[OSPI] memory-mapped mode FAILED\n");
        return BOOT_OSPI_ERR_INIT;
    }

    printf("[OSPI] memory-mapped mode OK\n");
    return BOOT_OSPI_OK;
}

#define OSPI_READ_CHUNK  4096U

BootOspiResult_t BootOspi_Read(uint32_t address, uint8_t *buffer, uint32_t len)
{
    uint32_t offset = 0U;
    uint32_t chunk;

    while (offset < len)
    {
        chunk = len - offset;
        if (chunk > OSPI_READ_CHUNK)
        {
            chunk = OSPI_READ_CHUNK;
        }

        if (MX25LM51245G_ReadSTR(&hospi1, MX25LM51245G_OPI_MODE,
                                  MX25LM51245G_4BYTES_SIZE,
                                  &buffer[offset], address + offset, chunk) != MX25LM51245G_OK)
        {
            printf("[OSPI] read FAILED at 0x%08lX len=%lu\n",
                   (unsigned long)(address + offset), (unsigned long)chunk);
            return BOOT_OSPI_ERR_READ;
        }

        offset += chunk;
    }

    return BOOT_OSPI_OK;
}
