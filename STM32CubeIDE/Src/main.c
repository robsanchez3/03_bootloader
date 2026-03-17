/**
  * @file   main.c
  * @brief  O3 Bootloader — Phase 1
  *
  *  Behaviour:
  *    1. Minimal HW init (clocks, GPIO).
  *    2. Check for a valid application at APP_BASE (0x08020000).
  *    3. If valid  → jump to the application.
  *    4. If invalid → blink LED and wait in recovery loop
  *                    (USB update will be added in Phase 2).
  *
  *  NOTE: The application has NOT been moved yet at this stage.
  *        APP_BASE (0x08020000) will be erased/blank until Phase 1b
  *        (linker change in app project) is done.  The bootloader will
  *        correctly land in recovery mode until then.
  */

#include "stm32u5xx_hal.h"
#include "main.h"
#include "boot_jump.h"
#include <stdio.h>


/* -----------------------------------------------------------------------
 * Private function prototypes
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void Recovery_Loop(void);

int _write(int file, char *ptr, int len)
{
    int i;

    (void)file;

    for (i = 0; i < len; i++)
    {
        ITM_SendChar(*ptr++);
    }

    return len;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    /* 1 — HAL and system init */
    HAL_Init();
    /* Phase 1 skeleton: keep bring-up independent from board power config. */
    /* SystemPower_Config(); */
    SystemClock_Config();
    MX_GPIO_Init();

    printf("BL start\n");

    /* 2 — Check for a valid application */
    if (Boot_IsApplicationValid(APP_BASE))
    {
        /* 3 — Valid: jump (does not return) */
        printf("Jumping to app\n");
        Boot_JumpToApplication(APP_BASE);
    }

    /* 4 — No valid application found: enter recovery loop */
    printf("Recovery loop\n");
    Recovery_Loop();

    /* Should never reach here */
    while (1) {}
}

/* -----------------------------------------------------------------------
 * Recovery_Loop
 *   Blink the status LED to signal that no valid application was found.
 *   USB update logic will be inserted here in Phase 2.
 * ----------------------------------------------------------------------- */
static void Recovery_Loop(void)
{
    while (1)
    {
        HAL_GPIO_TogglePin(BOOT_LED_GPIO_Port, BOOT_LED_Pin);
        HAL_Delay(250);
    }
}

/* -----------------------------------------------------------------------
 * SystemPower_Config
 *   Same supply configuration as the application.
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void)
{
    HAL_PWREx_DisableUCPDDeadBattery();

    if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -----------------------------------------------------------------------
 * SystemClock_Config
 *   Minimal Phase 1 clock tree:
 *     HSI internal oscillator as SYSCLK, no PLL, no external dependencies.
 * ----------------------------------------------------------------------- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2  |
                                       RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -----------------------------------------------------------------------
 * MX_GPIO_Init
 *   Minimal GPIO init for the bootloader:
 *     - FS_PW_SW (PI15): output HIGH → VBUS OFF (active-low switch)
 *     - BOOT_LED (PB14): output LOW  → LED off initially
 * ----------------------------------------------------------------------- */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* FS_PW_SW → HIGH = VBUS OFF */
    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = FS_PW_SW_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(FS_PW_SW_GPIO_Port, &GPIO_InitStruct);

    /* BOOT_LED → LOW = off */
    HAL_GPIO_WritePin(BOOT_LED_GPIO_Port, BOOT_LED_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = BOOT_LED_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOOT_LED_GPIO_Port, &GPIO_InitStruct);
}

/* -----------------------------------------------------------------------
 * Error_Handler
 * ----------------------------------------------------------------------- */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}


#if 0
/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

int main(void)
{
    /* Loop forever */
	for(;;);
}
#endif


#if 0
/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

int main(void)
{
    /* Loop forever */
	for(;;);
}
#endif
