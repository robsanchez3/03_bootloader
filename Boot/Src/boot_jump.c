#include "boot_jump.h"
#include "main.h"
#include <stdio.h>

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

    printf("[BOOT] Validate app @ 0x%08lX\n", (unsigned long)app_base);
    printf("[BOOT] MSP=0x%08lX RESET=0x%08lX\n", (unsigned long)msp, (unsigned long)reset_vector);

    if (msp < RAM_BASE || msp > RAM_TOP)
    {
        printf("[BOOT] Invalid app: MSP out of RAM range\n");
        return 0;
    }

    if ((msp & 0x7U) != 0U)
    {
        printf("[BOOT] Invalid app: MSP not 8-byte aligned\n");
        return 0;
    }

    if ((reset_vector & 0x1U) == 0U)
    {
        printf("[BOOT] Invalid app: reset vector Thumb bit not set\n");
        return 0;
    }

    reset_addr = reset_vector & ~1U;
    if (reset_addr < app_base || reset_addr > APP_END)
    {
        printf("[BOOT] Invalid app: reset vector out of app range\n");
        return 0;
    }

    printf("[BOOT] Application image looks valid\n");
    return 1;
}

void Boot_JumpToApplication(uint32_t app_base)
{
    uint32_t app_msp;
    uint32_t app_reset_vector;

    printf("[BOOT] Jump requested to app @ 0x%08lX\n", (unsigned long)app_base);

    printf("[BOOT] Disable IRQs\n");
    __disable_irq();
    Boot_DisableAllInterrupts();

    printf("[BOOT] Stop SysTick\n");
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    app_msp = *(volatile uint32_t *)app_base;
    app_reset_vector = *(volatile uint32_t *)(app_base + 4U);
    printf("[BOOT] App MSP=0x%08lX RESET=0x%08lX\n",
           (unsigned long)app_msp,
           (unsigned long)app_reset_vector);

    printf("[BOOT] Relocate VTOR to 0x%08lX\n", (unsigned long)app_base);
    SCB->VTOR = app_base;
    __DSB();
    __ISB();

    printf("[BOOT] Restore core state and MSP\n");
    __set_CONTROL(0U);
    __set_BASEPRI(0U);
    __set_FAULTMASK(0U);
    __set_MSPLIM(0U);
    __set_PSPLIM(0U);
    __ISB();

    printf("[BOOT] Re-enable global IRQs and branch to reset handler\n");
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
