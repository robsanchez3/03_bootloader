/**
  * @brief CMSIS Device System Source File for STM32U5xx — O3 Bootloader
  *
  *  Identical to the application version.
  *  VECT_TAB_OFFSET = 0x00000000 → SCB->VTOR = 0x08000000 (bootloader base).
  *  The bootloader relocates VTOR to APP_BASE (0x08020000) just before jumping.
  */

#include "stm32u5xx.h"

#if !defined(HSE_VALUE)
  #define HSE_VALUE  25000000U
#endif

#if !defined(HSI_VALUE)
  #define HSI_VALUE  16000000U
#endif

#if !defined(MSI_VALUE)
  #define MSI_VALUE  4000000U
#endif

/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00000000UL

uint32_t SystemCoreClock = 4000000U;

const uint8_t  AHBPrescTable[16] = {0U,0U,0U,0U,0U,0U,0U,0U,1U,2U,3U,4U,6U,7U,8U,9U};
const uint8_t  APBPrescTable[8]  = {0U,0U,0U,0U,1U,2U,3U,4U};
const uint32_t MSIRangeTable[16] = {
    48000000U, 24000000U, 16000000U, 12000000U,
     4000000U,  2000000U,  1330000U,  1000000U,
     3072000U,  1536000U,  1024000U,   768000U,
      400000U,   200000U,   133000U,   100000U
};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 20U) | (3UL << 22U));
#endif

    RCC->CR = RCC_CR_MSISON;
    RCC->CFGR1 = 0U;
    RCC->CFGR2 = 0U;
    RCC->CFGR3 = 0U;
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON);
    RCC->PLL1CFGR = 0U;
    RCC->CR &= ~(RCC_CR_HSEBYP);
    RCC->CIER = 0U;

#ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM1_BASE | VECT_TAB_OFFSET;
#else
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;   /* 0x08000000 — bootloader base */
#endif
}

void SystemCoreClockUpdate(void)
{
    /* Minimal stub — HAL_RCC_GetHCLKFreq() is preferred at runtime */
    SystemCoreClock = HAL_RCC_GetHCLKFreq();
}
