#include "boot_crc.h"
#include "stm32u5xx_hal.h"

static CRC_HandleTypeDef hcrc;
static uint8_t hcrc_initialized = 0U;

void BootCrc32_Init(void)
{
    if (hcrc_initialized != 0U)
    {
        return;
    }

    __HAL_RCC_CRC_CLK_ENABLE();

    hcrc.Instance                     = CRC;
    hcrc.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_BYTE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_ENABLE;
    hcrc.InputDataFormat              = CRC_INPUTDATA_FORMAT_BYTES;

    if (HAL_CRC_Init(&hcrc) == HAL_OK)
    {
        hcrc_initialized = 1U;
    }
}

uint32_t BootCrc32_Compute(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    uint32_t raw;

    BootCrc32_Init();

    if (crc == 0U)
    {
        raw = HAL_CRC_Calculate(&hcrc, (uint32_t *)buf, len);
    }
    else
    {
        raw = HAL_CRC_Accumulate(&hcrc, (uint32_t *)buf, len);
    }

    return raw ^ 0xFFFFFFFFU;
}

void BootCrc32_DeInit(void)
{
    if (hcrc_initialized != 0U)
    {
        HAL_CRC_DeInit(&hcrc);
        __HAL_RCC_CRC_CLK_DISABLE();
        hcrc_initialized = 0U;
    }
}
