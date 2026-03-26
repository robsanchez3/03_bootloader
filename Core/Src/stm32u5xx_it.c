/**
  * @brief Interrupt Service Routines — O3 Bootloader (minimal set)
  *
  *  Phase 1 baseline: only fault handlers + SysTick for HAL_IncTick().
  *  The bootloader uses SysTick as HAL timebase (not TIM2 like the app).
  */

#include "main.h"
#include "stm32u5xx_hal.h"

extern HCD_HandleTypeDef hhcd_USB_OTG_HS;

void OTG_HS_IRQHandler(void)
{
    HAL_HCD_IRQHandler(&hhcd_USB_OTG_HS);
}

void NMI_Handler(void)
{
    while (1) {}
}

void HardFault_Handler(void)
{
    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void DebugMon_Handler(void)
{
}

/**
  * @brief SysTick drives HAL_IncTick() in the bootloader.
  *        (The application uses TIM2 instead, but the bootloader
  *         keeps it simple with the default SysTick timebase.)
  */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
