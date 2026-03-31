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
#include "usb_msc_service.h"
#include "usb_fs_service.h"
#include "usb_update.h"
#include "ff.h"
#include "usbh_diskio.h"
#include <stdio.h>


/* -----------------------------------------------------------------------
 * Private function prototypes
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void Recovery_Loop(void);
static void Boot_RunStartupPolicy(void);
static void UsbMscLayerTestLoop(void);
static void ReadBinTest(const char *bin_path, uint32_t expected_crc32);
static void ReadBinChunkedTest(const char *bin_path, uint32_t expected_crc32);
static void UsbFsMountTestLoop(void);

#define BOOT_FORCE_INVALID_APP_FOR_TEST   0U
#define BOOT_USB_MSC_LAYER_TEST           0U
#define BOOT_USB_FS_MOUNT_TEST            1U

/* Chunked read configuration — tune these values during testing */
#define CHUNKED_READ_BLOCK_SIZE           1048576U /* bytes per open/read/close cycle */
#define CHUNKED_READ_MAX_RETRIES          10U      /* retries per block on failure    */

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


    printf("BL start\n");

#if BOOT_USB_MSC_LAYER_TEST
    UsbMscLayerTestLoop();
#endif

#if BOOT_USB_FS_MOUNT_TEST
    UsbFsMountTestLoop();
#endif

    Boot_RunStartupPolicy();

    /* Should never reach here */
    while (1) {}
}

/* -----------------------------------------------------------------------
 * Recovery_Loop
 *   Blink the status LED to signal that no valid application was found.
 * ----------------------------------------------------------------------- */
static void Recovery_Loop(void)
{
    uint32_t last_blink = HAL_GetTick();

    while (1)
    {
        if ((HAL_GetTick() - last_blink) >= 250U)
        {
            last_blink = HAL_GetTick();
            HAL_GPIO_TogglePin(BOOT_LED_GPIO_Port, BOOT_LED_Pin);
        }
    }
}

static void Boot_RunStartupPolicy(void)
{
    uint8_t app_valid;

    app_valid = (uint8_t)Boot_IsApplicationValid(APP_BASE);

#if BOOT_FORCE_INVALID_APP_FOR_TEST
    printf("[BOOT] TEST MODE: forcing application invalid\n");
    app_valid = 0U;
#endif

    if (app_valid != 0U)
    {
        printf("[BOOT] Valid app found -> jump\n");
        Boot_JumpToApplication(APP_BASE);
    }

    printf("[BOOT] No valid app -> recovery loop\n");
    Recovery_Loop();
}

static void UsbMscLayerTestLoop(void)
{
    UsbMscState_t last_state;

    printf("[TEST] USB MSC layer test enabled\n");

    UsbMscService_Init();
    UsbMscService_SetEnabled(1U);
    last_state = UsbMscService_GetState();
    printf("[TEST] USB state: %s\n", UsbMscService_GetStateName());

    while (1)
    {
        UsbMscService_Process();

        if (UsbMscService_GetState() != last_state)
        {
            last_state = UsbMscService_GetState();
            printf("[TEST] USB state: %s\n", UsbMscService_GetStateName());

            if (last_state == USB_MSC_STATE_ERROR)
            {
                printf("[TEST] USB error code: %lu\n",
                       (unsigned long)UsbMscService_GetLastError());
            }
        }
    }
}

/* CRC32 IEEE 802.3 por nibbles (tabla de 16 entradas, sin heap). */
static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    static const uint32_t T[16] =
    {
        0x00000000U, 0x1DB71064U, 0x3B6E20C8U, 0x26D930ACU,
        0x76DC4190U, 0x6B6B51F4U, 0x4DB26158U, 0x5005713CU,
        0xEDB88320U, 0xF00F9344U, 0xD6D6A3E8U, 0xCB61B38CU,
        0x9B64C2B0U, 0x86D3D2D4U, 0xA00AE278U, 0xBDBDF21CU
    };
    uint32_t i;

    crc = ~crc;
    for (i = 0U; i < len; i++)
    {
        crc = (crc >> 4) ^ T[(crc ^ (uint32_t)buf[i])        & 0x0FU];
        crc = (crc >> 4) ^ T[(crc ^ ((uint32_t)buf[i] >> 4)) & 0x0FU];
    }
    return ~crc;
}

/* Lee un .bin completo por bloques de 512 B, calcula CRC32 y compara. */
static void ReadBinTest(const char *bin_path, uint32_t expected_crc32)
{
    static uint8_t  chunk[65536];
    UsbFsResult_t   result;
    uint32_t        bytes_read;
    uint32_t        total     = 0U;
    uint32_t        crc       = 0U;
    uint8_t         first     = 1U;

    result = UsbFsService_OpenStream(bin_path);
    if (result != USB_FS_RESULT_OK)
    {
        printf("[TEST] open %s FAILED result=%u fatfs_err=%lu\n",
               bin_path, (unsigned int)result,
               (unsigned long)UsbFsService_GetLastError());
        return;
    }

    printf("[TEST] reading %s ...\n", bin_path);

    while (1)
    {
        result = UsbFsService_ReadStream(chunk, sizeof(chunk), &bytes_read);
        if (result != USB_FS_RESULT_OK)
        {
            printf("[TEST] read error at offset=%lu result=%u fatfs_err=%lu\n",
                   (unsigned long)total, (unsigned int)result,
                   (unsigned long)UsbFsService_GetLastError());
            (void)UsbFsService_CloseStream();
            return;
        }

        if (bytes_read == 0U)
        {
            break;  /* EOF */
        }

        if (first != 0U)
        {
            first = 0U;
            printf("[TEST] first 8 bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                   chunk[0], chunk[1], chunk[2], chunk[3],
                   chunk[4], chunk[5], chunk[6], chunk[7]);
        }

        crc    = crc32_update(crc, chunk, bytes_read);
        total += bytes_read;
    }

    (void)UsbFsService_CloseStream();

    printf("[TEST] %s: %lu bytes  CRC32=%08lX  expected=%08lX  %s\n",
           bin_path,
           (unsigned long)total,
           (unsigned long)crc,
           (unsigned long)expected_crc32,
           (crc == expected_crc32) ? "MATCH" : "MISMATCH");
}

/* Lee un .bin completo mediante open/seek/read/close por cada bloque.
 * El tamaño de bloque y reintentos se configuran con CHUNKED_READ_BLOCK_SIZE
 * y CHUNKED_READ_MAX_RETRIES. */
static void ReadBinChunkedTest(const char *bin_path, uint32_t expected_crc32)
{
    static uint8_t  chunk[CHUNKED_READ_BLOCK_SIZE];
    UsbFsResult_t   result;
    uint32_t        file_size   = 0U;
    uint32_t        offset      = 0U;
    uint32_t        bytes_read;
    uint32_t        to_read;
    uint32_t        crc         = 0U;
    uint32_t        block_num   = 0U;
    uint32_t        retry;
    uint32_t        total_retries = 0U;
    uint32_t        t_start;

    result = UsbFsService_GetFileSize(bin_path, &file_size);
    if (result != USB_FS_RESULT_OK)
    {
        printf("[CHUNK] %s GetFileSize FAILED result=%u\n",
               bin_path, (unsigned int)result);
        return;
    }

    printf("[CHUNK] %s size=%lu  block=%u  blocks=%lu\n",
           bin_path,
           (unsigned long)file_size,
           (unsigned int)CHUNKED_READ_BLOCK_SIZE,
           (unsigned long)((file_size + CHUNKED_READ_BLOCK_SIZE - 1U) / CHUNKED_READ_BLOCK_SIZE));

    t_start = HAL_GetTick();

    while (offset < file_size)
    {
        to_read = file_size - offset;
        if (to_read > CHUNKED_READ_BLOCK_SIZE)
        {
            to_read = CHUNKED_READ_BLOCK_SIZE;
        }

        bytes_read = 0U;
        for (retry = 0U; retry <= CHUNKED_READ_MAX_RETRIES; retry++)
        {
            result = UsbFsService_ReadFile(bin_path, chunk, offset, to_read, &bytes_read);
            if (result == USB_FS_RESULT_OK)
            {
                break;
            }
            if (retry < CHUNKED_READ_MAX_RETRIES)
            {
                total_retries++;
                printf("[CHUNK] block %lu offset=%lu RETRY %lu/%u err=%u fatfs=%lu -> recovery\n",
                       (unsigned long)block_num,
                       (unsigned long)offset,
                       (unsigned long)(retry + 1U),
                       (unsigned int)CHUNKED_READ_MAX_RETRIES,
                       (unsigned int)result,
                       (unsigned long)UsbFsService_GetLastError());

                /* Full recovery: unmount + USB host restart + wait READY + remount */
                UsbFsService_Unmount();
                UsbMscService_ForceRestart();

                {
                    uint32_t t_wait = HAL_GetTick();
                    while (UsbMscService_IsReady() == 0U)
                    {
                        UsbMscService_Process();
                        if ((HAL_GetTick() - t_wait) > 10000U)
                        {
                            printf("[CHUNK] recovery timeout waiting for READY\n");
                            break;
                        }
                    }
                }

                if (UsbMscService_IsReady() != 0U)
                {
                    if (UsbFsService_Mount() != USB_FS_RESULT_OK)
                    {
                        printf("[CHUNK] remount FAILED fatfs=%lu\n",
                               (unsigned long)UsbFsService_GetLastError());
                    }
                    else
                    {
                        printf("[CHUNK] recovery OK -> remounted\n");
                    }
                }
            }
        }

        if (result != USB_FS_RESULT_OK)
        {
            printf("[CHUNK] block %lu offset=%lu FAILED after %u retries  result=%u fatfs=%lu\n",
                   (unsigned long)block_num,
                   (unsigned long)offset,
                   (unsigned int)CHUNKED_READ_MAX_RETRIES,
                   (unsigned int)result,
                   (unsigned long)UsbFsService_GetLastError());
            return;
        }

        if (bytes_read != to_read)
        {
            printf("[CHUNK] block %lu offset=%lu short read: expected=%lu got=%lu\n",
                   (unsigned long)block_num,
                   (unsigned long)offset,
                   (unsigned long)to_read,
                   (unsigned long)bytes_read);
            return;
        }

        crc     = crc32_update(crc, chunk, bytes_read);
        offset += bytes_read;
        block_num++;

        /* Pausa entre bloques para estabilizar el controlador USB */
        if (offset < file_size)
        {
            HAL_Delay(500U);
        }

        /* Progreso cada 32 bloques */
        if ((block_num & 1U) == 0U)
        {
            printf("[CHUNK] %lu / %lu bytes (%lu%%)\n",
                   (unsigned long)offset,
                   (unsigned long)file_size,
                   (unsigned long)((offset * 100U) / file_size));
        }
    }

    uint32_t elapsed = HAL_GetTick() - t_start;

    printf("[CHUNK] %s: %lu bytes  CRC32=%08lX  expected=%08lX  %s  %lu ms  retries=%lu\n",
           bin_path,
           (unsigned long)offset,
           (unsigned long)crc,
           (unsigned long)expected_crc32,
           (crc == expected_crc32) ? "MATCH" : "MISMATCH",
           (unsigned long)elapsed,
           (unsigned long)total_retries);
}

static void UsbFsMountTestLoop(void)
{
    UsbMscState_t last_state;
    UsbFsResult_t mount_result;

    printf("[TEST] USB FS mount test enabled\n");

    UsbMscService_Init();
    UsbMscService_SetEnabled(1U);
    UsbFsService_Init();

    last_state = UsbMscService_GetState();
    printf("[TEST] USB state: %s\n", UsbMscService_GetStateName());

    while (1)
    {
        UsbMscService_Process();

        if (UsbMscService_GetState() != last_state)
        {
            last_state = UsbMscService_GetState();
            printf("[TEST] USB state: %s\n", UsbMscService_GetStateName());

            if (last_state == USB_MSC_STATE_READY)
            {
                static const char * const probe_paths[4] =
                {
                    USB_UPDATE_APP_INT_BIN,
                    USB_UPDATE_APP_OSPI_BIN,
                    USB_UPDATE_APP_INT_CRC,
                    USB_UPDATE_APP_OSPI_CRC
                };
                uint32_t i;
                UsbFsResult_t stat_result;
                uint8_t all_present;

                printf("[TEST] MSC ready -> attempting f_mount\n");
                mount_result = UsbFsService_Mount();

                if (mount_result != USB_FS_RESULT_OK)
                {
                    printf("[TEST] f_mount FAILED result=%u fatfs_err=%lu\n",
                           (unsigned int)mount_result,
                           (unsigned long)UsbFsService_GetLastError());
                }
                else
                {
                    printf("[TEST] f_mount OK\n");

                    {
                        FATFS  *fs_ptr;
                        DWORD   free_clust;
                        if (f_getfree("0:", &free_clust, &fs_ptr) == FR_OK)
                        {
                            printf("[TEST] sector=%u bytes  cluster=%u sectors (%lu bytes)\n",
                                   (unsigned int)FF_MAX_SS,
                                   (unsigned int)fs_ptr->csize,
                                   (unsigned long)((uint32_t)fs_ptr->csize * FF_MAX_SS));
                        }
                    }

                    stat_result = UsbFsService_FileExists(USB_UPDATE_DIR_PATH);
                    printf("[TEST] f_stat dir %s -> %s (fatfs_err=%lu)\n",
                           USB_UPDATE_DIR_PATH,
                           (stat_result == USB_FS_RESULT_OK) ? "OK" : "NOT FOUND",
                           (unsigned long)UsbFsService_GetLastError());

                    all_present = 1U;
                    for (i = 0U; i < 4U; i++)
                    {
                        stat_result = UsbFsService_FileExists(probe_paths[i]);
                        printf("[TEST] f_stat %s -> %s (fatfs_err=%lu)\n",
                               probe_paths[i],
                               (stat_result == USB_FS_RESULT_OK) ? "OK" : "NOT FOUND",
                               (unsigned long)UsbFsService_GetLastError());
                        if (stat_result != USB_FS_RESULT_OK)
                        {
                            all_present = 0U;
                        }
                    }

                    if (all_present != 0U)
                    {
                        uint32_t fsize_tmp;
                        if (UsbFsService_GetFileSize(USB_UPDATE_APP_INT_BIN, &fsize_tmp) == USB_FS_RESULT_OK)
                            printf("[TEST] %s size=%lu bytes\n", USB_UPDATE_APP_INT_BIN, (unsigned long)fsize_tmp);
                        if (UsbFsService_GetFileSize(USB_UPDATE_APP_OSPI_BIN, &fsize_tmp) == USB_FS_RESULT_OK)
                            printf("[TEST] %s size=%lu bytes\n", USB_UPDATE_APP_OSPI_BIN, (unsigned long)fsize_tmp);

                        static const char * const crc_paths[2] =
                        {
                            USB_UPDATE_APP_INT_CRC,
                            USB_UPDATE_APP_OSPI_CRC
                        };
                        uint8_t  crc_buf[16];
                        uint32_t bytes_read;
                        uint32_t j;
                        UsbFsResult_t read_result;

                        uint32_t expected_crc[2] = { 0U, 0U };

                        printf("[TEST] all 4 files present\n");

                        /* Leer los dos .crc y parsear el valor hex ASCII */
                        for (i = 0U; i < 2U; i++)
                        {
                            bytes_read  = 0U;
                            read_result = UsbFsService_ReadFile(crc_paths[i],
                                                                crc_buf,
                                                                0U,
                                                                sizeof(crc_buf),
                                                                &bytes_read);
                            if (read_result == USB_FS_RESULT_OK)
                            {
                                uint32_t val = 0U;
                                uint32_t k;

                                for (k = 0U; k < bytes_read; k++)
                                {
                                    uint8_t c = crc_buf[k];
                                    if ((c >= '0') && (c <= '9'))
                                        val = (val << 4) | (uint32_t)(c - '0');
                                    else if ((c >= 'A') && (c <= 'F'))
                                        val = (val << 4) | (uint32_t)(c - 'A' + 10);
                                    else if ((c >= 'a') && (c <= 'f'))
                                        val = (val << 4) | (uint32_t)(c - 'a' + 10);
                                    else
                                        break;
                                }
                                expected_crc[i] = val;
                                printf("[TEST] %s -> 0x%08lX\n",
                                       crc_paths[i], (unsigned long)val);
                            }
                            else
                            {
                                printf("[TEST] read %s FAILED result=%u fatfs_err=%lu\n",
                                       crc_paths[i],
                                       (unsigned int)read_result,
                                       (unsigned long)UsbFsService_GetLastError());
                            }
                        }

                        /* Leer los dos .bin con chunked reopen y verificar CRC32 */
                        ReadBinChunkedTest(USB_UPDATE_APP_INT_BIN,  expected_crc[0]);
                        ReadBinChunkedTest(USB_UPDATE_APP_OSPI_BIN, expected_crc[1]);
                    }
                    else
                    {
                        printf("[TEST] one or more files missing\n");
                    }
                }
            }
            else if (last_state == USB_MSC_STATE_IDLE)
            {
                printf("[TEST] device disconnected -> unmounting\n");
                UsbFsService_Unmount();
            }
        }
    }
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
