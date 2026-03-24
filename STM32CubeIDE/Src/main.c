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
#include "usb_host.h"
#include "fatfs_usb.h"
#include <stdio.h>


/* -----------------------------------------------------------------------
 * Private function prototypes
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void Recovery_Loop(void);
static uint8_t Boot_WaitUsbAtStart(uint32_t timeout_ms);
static uint8_t Boot_WaitForValidUpdate(void);
static void Boot_RunStartupPolicy(void);

#define BOOT_USB_START_WINDOW_MS   3000U
#define BOOT_FORCE_INVALID_APP_FOR_TEST   0U

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
    __HAL_RCC_PWR_CLK_ENABLE();
    SystemPower_Config();
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
    {
        Error_Handler();
    }
    SystemClock_Config();
    MX_GPIO_Init();

    printf("FATFS init...\n");
    MX_FATFS_USB_Init();

    printf("USB host init...\n");
    MX_USB_HOST_Init();
    printf("BL start\n");

    Boot_RunStartupPolicy();

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
    uint32_t last_blink = HAL_GetTick();
    uint8_t update_detected = 0U;

    while (1)
    {
        MX_USB_HOST_Process();
        USBH_AppTask();

        if ((update_detected == 0U) && (Boot_WaitForValidUpdate() != 0U))
        {
            update_detected = 1U;
            printf("[BOOT] Update media detected in recovery\n");
            /* Programming flow will be inserted here in the next phase. */
        }

        if (update_detected != 0U)
        {
            continue;
        }

        if ((HAL_GetTick() - last_blink) >= 250U)
        {
            last_blink = HAL_GetTick();
            HAL_GPIO_TogglePin(BOOT_LED_GPIO_Port, BOOT_LED_Pin);
        }
    }
}

static uint8_t Boot_WaitUsbAtStart(uint32_t timeout_ms)
{
    uint32_t deadline = HAL_GetTick() + timeout_ms;

    USBH_ClearUpdateDetection();

    while ((int32_t)(HAL_GetTick() - deadline) < 0)
    {
        MX_USB_HOST_Process();
        USBH_AppTask();

        if (USBH_HasPhysicalConnection() != 0U)
        {
            printf("[BOOT] USB physical connection detected during startup window\n");
            return 1U;
        }
    }

    printf("[BOOT] No USB physical connection detected during startup window\n");
    return 0U;
}

static uint8_t Boot_WaitForValidUpdate(void)
{
    while (1)
    {
        MX_USB_HOST_Process();
        USBH_AppTask();

        if (USBH_HasValidUpdateImage() != 0U)
        {
            printf("[BOOT] Found valid update set: app_int/app_ospi + CRCs\n");
            return 1U;
        }

        if (USBH_IsUpdateCheckComplete() != 0U)
        {
            return 0U;
        }
    }
}

static void Boot_RunStartupPolicy(void)
{
    uint8_t app_valid;

    if (Boot_WaitUsbAtStart(BOOT_USB_START_WINDOW_MS) != 0U)
    {
        if (Boot_WaitForValidUpdate() != 0U)
        {
            printf("[BOOT] Valid update detected at startup\n");
            printf("[BOOT] Programming flow not implemented yet -> recovery\n");
            Recovery_Loop();
        }

        printf("[BOOT] USB present but no valid update -> recovery\n");
        Recovery_Loop();
    }

    if (USBH_WasUsbSeen() != 0U)
    {
        printf("[BOOT] USB activity seen during startup -> recovery\n");
        Recovery_Loop();
    }

    app_valid = (uint8_t)Boot_IsApplicationValid(APP_BASE);

#if BOOT_FORCE_INVALID_APP_FOR_TEST
    printf("[BOOT] TEST MODE: forcing application invalid\n");
    app_valid = 0U;
#endif

    if (app_valid != 0U)
    {
        printf("Jumping to app\n");
        Boot_JumpToApplication(APP_BASE);
    }

    printf("Recovery loop\n");
    Recovery_Loop();
}

/* -----------------------------------------------------------------------
 * SystemPower_Config
 *   Same supply configuration as the application.
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void)
{
//    HAL_PWREx_DisableUCPDDeadBattery();

    if (HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -----------------------------------------------------------------------
 * SystemClock_Config
 *   Keep SYSCLK on HSI for stable bring-up, but enable HSE + PLL1 so the
 *   USB PHY can use a valid dedicated clock source.
 * ----------------------------------------------------------------------- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMBOOST  = RCC_PLLMBOOST_DIV2;
    RCC_OscInitStruct.PLL.PLLM       = 5;
    RCC_OscInitStruct.PLL.PLLN       = 64;
    RCC_OscInitStruct.PLL.PLLP       = 10;
    RCC_OscInitStruct.PLL.PLLQ       = 2;
    RCC_OscInitStruct.PLL.PLLR       = 2;
    RCC_OscInitStruct.PLL.PLLRGE     = RCC_PLLVCIRANGE_0;
    RCC_OscInitStruct.PLL.PLLFRACN   = 0;
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
