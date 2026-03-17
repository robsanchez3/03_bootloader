#include "boot_jump.h"
#include "main.h"

/* Pointer-to-function type for the application reset handler */
typedef void (*pFunction)(void);

int Boot_IsApplicationValid(uint32_t app_base)
{
#if 0    
    uint32_t msp         = *(volatile uint32_t *)app_base;
    uint32_t reset_vector = *(volatile uint32_t *)(app_base + 4U);

    /* MSP must point inside RAM */
    if (msp < RAM_BASE || msp > RAM_END)
    {
        return 0;
    }

    /* Reset vector must point inside the application flash region
     * (bit 0 is the Thumb bit, mask it out before range check) */
    uint32_t reset_addr = reset_vector & ~1U;
    if (reset_addr < app_base || reset_addr > APP_END)
    {
        return 0;
    }
#endif
    return 1;
}

void Boot_JumpToApplication(uint32_t app_base)
{
#if 0    
    pFunction app_reset_handler;

    /* Disable all interrupts and clear pending bits */
    __disable_irq();

    /* Stop SysTick */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* De-init HAL (disables clocks, resets peripherals) */
    HAL_RCC_DeInit();
    HAL_DeInit();

    /* Relocate vector table to application base */
    SCB->VTOR = app_base;

    /* Set the stack pointer to the application's initial MSP */
    __set_MSP(*(volatile uint32_t *)app_base);

    /* Get the application reset handler address */
    app_reset_handler = (pFunction)(*(volatile uint32_t *)(app_base + 4U));

    /* Jump — does not return */
    app_reset_handler();
#endif
    //printf("Should never reach here... \n");
    /* Should never reach here */
    while (1) {}

}
