/**
  * @file   main.c
  * @brief  O3 Bootloader
  *
  *  Behaviour:
  *    1. Minimal HW init (clocks, GPIO).
  *    2. Wait for USB pendrive, read update files.
  *    3. Program internal flash (app_int.bin) and OSPI flash (app_ospi.bin).
  *    4. Verify CRC32 of both flashes.
  *    5. Jump to application at APP_BASE (0x08020000).
  */

#include "stm32u5xx_hal.h"
#include "main.h"
#include "boot_jump.h"
#include "boot_flash.h"
#include "boot_ospi.h"
#include "boot_display.h"
#include "usb_msc_service.h"
#include "usb_fs_service.h"
#include "usb_update.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>


/* -----------------------------------------------------------------------
 * Private function prototypes
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void Recovery_Loop(void);
static void Boot_RunStartupPolicy(void);
static void FlashAppInt(uint32_t expected_crc32);
static void FlashAppOspi(uint32_t expected_crc32);
static void UsbUpdateLoop(void);
static uint8_t UsbBootProbe(uint32_t detect_timeout_ms, uint32_t ready_timeout_ms);
static void BootDisplay_EnsureInit(void);
static void BootDisplay_UpdateProgress(const char *label, uint32_t offset, uint32_t total, uint32_t step_bytes);
static void BootDisplay_FlashEraseProgress(uint32_t current, uint32_t total);
static void BootDisplay_OspiEraseProgress(uint32_t current, uint32_t total);

#define BOOT_FORCE_INVALID_APP_FOR_TEST   0U
#define BOOT_USB_UPDATE_ENABLED           1U
#define BOOT_USB_DETECT_TIMEOUT_MS        1800U
#define BOOT_USB_READY_TIMEOUT_MS         2500U

/* Chunked read configuration */
#define CHUNKED_READ_BLOCK_SIZE           1048576U /* bytes per open/read/close cycle */
#define CHUNKED_READ_MAX_RETRIES          20U      /* retries per block on failure    */
#define CHUNKED_READ_INTER_BLOCK_DELAY_MS 500U     /* pause between blocks            */

/* Single shared IO buffer — used by FlashAppInt and FlashAppOspi.
 * These functions never run concurrently so one buffer is sufficient. */
static uint8_t io_buf[CHUNKED_READ_BLOCK_SIZE];
static uint8_t boot_display_initialized;

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
 * CRC32 IEEE 802.3 using STM32 hardware CRC peripheral.
 * ----------------------------------------------------------------------- */
static CRC_HandleTypeDef hcrc;
static uint8_t hcrc_initialized = 0U;

static void crc32_hw_init(void)
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

static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    uint32_t raw;

    crc32_hw_init();

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

static void crc32_deinit(void)
{
    if (hcrc_initialized != 0U)
    {
        HAL_CRC_DeInit(&hcrc);
        __HAL_RCC_CRC_CLK_DISABLE();
        hcrc_initialized = 0U;
    }
}

static void BootDisplay_EnsureInit(void)
{
    if (boot_display_initialized == 0U)
    {
        BootDisplay_Init();
        boot_display_initialized = 1U;
    }
}

static uint8_t UsbBootProbe(uint32_t detect_timeout_ms, uint32_t ready_timeout_ms)
{
    uint32_t start = HAL_GetTick();
    UsbMscState_t state;

    UsbMscService_Init();
    UsbMscService_SetEnabled(1U);
    UsbFsService_Init();

    while ((HAL_GetTick() - start) < detect_timeout_ms)
    {
        UsbMscService_Process();
        state = UsbMscService_GetState();

        if (state == USB_MSC_STATE_READY)
        {
            return 1U;
        }

        if ((state == USB_MSC_STATE_DEVICE_CONNECTED) ||
            (state == USB_MSC_STATE_ENUMERATING))
        {
            uint32_t connected_start = HAL_GetTick();

            while ((HAL_GetTick() - connected_start) < ready_timeout_ms)
            {
                UsbMscService_Process();
                state = UsbMscService_GetState();

                if (state == USB_MSC_STATE_READY)
                {
                    return 1U;
                }

                if ((state == USB_MSC_STATE_IDLE) || (state == USB_MSC_STATE_OFF))
                {
                    break;
                }
            }

            break;
        }
    }

    UsbFsService_Unmount();
    UsbMscService_SetEnabled(0U);
    return 0U;
}

static void BootDisplay_UpdateProgress(const char *label, uint32_t offset, uint32_t total, uint32_t step_bytes)
{
    uint32_t shown;

    (void)step_bytes;

    if (total == 0U)
    {
        BootDisplay_ShowProgress(label, 0U, 1U);
        return;
    }

    shown = offset;
    if (shown > total)
    {
        shown = total;
    }

    BootDisplay_ShowProgress(label, shown, total);
}

static void BootDisplay_FlashEraseProgress(uint32_t current, uint32_t total)
{
    BootDisplay_ShowProgress("ERASING INT FLASH...", current, total);
}

static void BootDisplay_OspiEraseProgress(uint32_t current, uint32_t total)
{
    BootDisplay_ShowProgress("ERASING OSPI FLASH...", current, total);
}

/* -----------------------------------------------------------------------
 * USB read helper: read a chunk with full USB restart recovery on failure.
 * ----------------------------------------------------------------------- */
static UsbFsResult_t UsbReadChunkWithRecovery(const char *path,
                                               uint8_t *buf,
                                               uint32_t offset,
                                               uint32_t to_read,
                                               uint32_t *bytes_read,
                                               const char *tag)
{
    UsbFsResult_t result;
    uint32_t retry;

    *bytes_read = 0U;
    for (retry = 0U; retry <= CHUNKED_READ_MAX_RETRIES; retry++)
    {
        result = UsbFsService_ReadFile(path, buf, offset, to_read, bytes_read);
        if (result == USB_FS_RESULT_OK)
        {
            return USB_FS_RESULT_OK;
        }
        if (retry < CHUNKED_READ_MAX_RETRIES)
        {
            printf("[%s] read offset=%lu RETRY %lu/%u -> recovery\n",
                   tag,
                   (unsigned long)offset,
                   (unsigned long)(retry + 1U),
                   (unsigned int)CHUNKED_READ_MAX_RETRIES);

            UsbFsService_Unmount();
            UsbMscService_ForceRestart();
            {
                uint32_t t_wait = HAL_GetTick();
                while (UsbMscService_IsReady() == 0U)
                {
                    UsbMscService_Process();
                    if ((HAL_GetTick() - t_wait) > 10000U)
                    {
                        printf("[%s] recovery timeout\n", tag);
                        break;
                    }
                }
            }
            if (UsbMscService_IsReady() != 0U)
            {
                (void)UsbFsService_Mount();
            }
        }
    }

    return result;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    uint8_t usb_boot = 0U;

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

#if BOOT_USB_UPDATE_ENABLED
    usb_boot = UsbBootProbe(BOOT_USB_DETECT_TIMEOUT_MS, BOOT_USB_READY_TIMEOUT_MS);
    if (usb_boot != 0U)
    {
        BootDisplay_EnsureInit();
        BootDisplay_Log("");
        BootDisplay_LogColor("BOOT START", BOOT_DISPLAY_COLOR_BLUE);
        UsbUpdateLoop();
    }
#endif

    Boot_RunStartupPolicy();

    while (1) {}
}

/* -----------------------------------------------------------------------
 * Recovery_Loop
 * ----------------------------------------------------------------------- */
static void Recovery_Loop(void)
{
    uint32_t last_blink = HAL_GetTick();

    BootDisplay_EnsureInit();
    BootDisplay_Log("RECOVERY MODE");
    BootDisplay_Log("NO VALID APP");

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
        UsbFsService_Unmount();
        UsbMscService_SetEnabled(0U);
        HAL_Delay(50U);
        printf("[BOOT] Valid app found -> jump\n");
        Boot_JumpToApplication(APP_BASE);
    }

    BootDisplay_EnsureInit();
    BootDisplay_Log("APP INVALID");
    BootDisplay_Log("ENTER RECOVERY");
    printf("[BOOT] No valid app -> recovery loop\n");
    Recovery_Loop();
}

/* -----------------------------------------------------------------------
 * FlashAppInt — Read app_int.bin from USB, program internal flash, verify.
 * ----------------------------------------------------------------------- */
static void FlashAppInt(uint32_t expected_crc32)
{
    uint8_t * const chunk = io_buf;
    UsbFsResult_t  result;
    BootFlashResult_t flash_result;
    uint32_t       file_size = 0U;
    uint32_t       offset    = 0U;
    uint32_t       bytes_read;
    uint32_t       to_read;
    uint32_t       t_start;

    printf("[FLASH] === app_int.bin -> internal flash ===\n");
    BootDisplay_LogColor("INT FLASH UPDATE", BOOT_DISPLAY_COLOR_BLUE);

    result = UsbFsService_GetFileSize(USB_UPDATE_APP_INT_BIN, &file_size);
    if (result != USB_FS_RESULT_OK)
    {
        BootDisplay_LogColor("INT FLASH FILE ERROR", BOOT_DISPLAY_COLOR_RED);
        printf("[FLASH] GetFileSize FAILED result=%u\n", (unsigned int)result);
        return;
    }
    printf("[FLASH] file size=%lu bytes\n", (unsigned long)file_size);

    printf("[FLASH] erasing...\n");
    BootDisplay_Log("ERASING INT FLASH...");
    BootDisplay_ShowProgress("ERASING INT FLASH...", 0U, 1U);
    flash_result = BootFlash_EraseAppArea(file_size, BootDisplay_FlashEraseProgress);
    if (flash_result != BOOT_FLASH_OK)
    {
        BootDisplay_LogColor("INT FLASH ERASE FAILED", BOOT_DISPLAY_COLOR_RED);
        BootDisplay_ClearProgress();
        printf("[FLASH] erase FAILED result=%u\n", (unsigned int)flash_result);
        return;
    }

    printf("[FLASH] programming...\n");
    BootDisplay_Log("INT FLASH ERASE DONE");
    BootDisplay_Log("WRITING INT FLASH...");
    BootDisplay_ClearProgress();
    BootDisplay_ShowProgress("WRITING INT FLASH...", 0U, file_size);
    t_start = HAL_GetTick();

    while (offset < file_size)
    {
        to_read = file_size - offset;
        if (to_read > CHUNKED_READ_BLOCK_SIZE)
        {
            to_read = CHUNKED_READ_BLOCK_SIZE;
        }

        result = UsbReadChunkWithRecovery(USB_UPDATE_APP_INT_BIN, chunk,
                                          offset, to_read, &bytes_read, "FLASH");
        if (result != USB_FS_RESULT_OK)
        {
            BootDisplay_LogColor("INT FLASH READ FAILED", BOOT_DISPLAY_COLOR_RED);
            BootDisplay_ClearProgress();
            printf("[FLASH] read FAILED at offset=%lu\n", (unsigned long)offset);
            return;
        }

        flash_result = BootFlash_Program(APP_BASE + offset, chunk, bytes_read);
        if (flash_result != BOOT_FLASH_OK)
        {
            BootDisplay_LogColor("INT FLASH WRITE FAILED", BOOT_DISPLAY_COLOR_RED);
            BootDisplay_ClearProgress();
            printf("[FLASH] program FAILED at offset=%lu result=%u\n",
                   (unsigned long)offset, (unsigned int)flash_result);
            return;
        }

        flash_result = BootFlash_Verify(APP_BASE + offset, chunk, bytes_read);
        if (flash_result != BOOT_FLASH_OK)
        {
            BootDisplay_LogColor("INT FLASH VERIFY FAILED", BOOT_DISPLAY_COLOR_RED);
            BootDisplay_ClearProgress();
            printf("[FLASH] verify FAILED at offset=%lu\n", (unsigned long)offset);
            return;
        }

        offset += bytes_read;
        BootDisplay_UpdateProgress("WRITING INT FLASH...", offset, file_size, CHUNKED_READ_BLOCK_SIZE);

        printf("[FLASH] %lu / %lu bytes (%lu%%)\n",
               (unsigned long)offset,
               (unsigned long)file_size,
               (unsigned long)((offset * 100U) / file_size));

        if (offset < file_size)
        {
            HAL_Delay(CHUNKED_READ_INTER_BLOCK_DELAY_MS);
        }
    }

    /* CRC32 over programmed flash */
    BootDisplay_Log("INT FLASH WRITE DONE");
    BootDisplay_Log("VERIFYING INT FLASH...");
    BootDisplay_ClearProgress();
    {
        uint32_t crc_flash = crc32_update(0U, (const uint8_t *)APP_BASE, file_size);
        BootDisplay_LogColor((crc_flash == expected_crc32) ? "INT FLASH CRC OK" : "INT FLASH CRC FAIL",
                             (crc_flash == expected_crc32) ? BOOT_DISPLAY_COLOR_NORMAL : BOOT_DISPLAY_COLOR_RED);

        printf("[FLASH] CRC32=%08lX  expected=%08lX  %s\n",
               (unsigned long)crc_flash,
               (unsigned long)expected_crc32,
               (crc_flash == expected_crc32) ? "MATCH" : "MISMATCH");
    }

    printf("[FLASH] Boot_IsApplicationValid: %s\n",
           Boot_IsApplicationValid(APP_BASE) ? "VALID" : "INVALID");

    printf("[FLASH] done in %lu ms\n", (unsigned long)(HAL_GetTick() - t_start));
}

/* -----------------------------------------------------------------------
 * FlashAppOspi — Read app_ospi.bin from USB, program OSPI flash, verify,
 *                enable memory-mapped mode and jump to app.
 * ----------------------------------------------------------------------- */
static void FlashAppOspi(uint32_t expected_crc32)
{
    uint8_t * const chunk = io_buf;
    UsbFsResult_t  result;
    BootOspiResult_t ospi_result;
    uint32_t       file_size = 0U;
    uint32_t       offset    = 0U;
    uint32_t       bytes_read;
    uint32_t       to_read;
    uint32_t       t_start;

    printf("[OSPI] === app_ospi.bin -> OSPI flash ===\n");
    BootDisplay_LogColor("OSPI FLASH UPDATE", BOOT_DISPLAY_COLOR_BLUE);
    BootDisplay_Log("INITIALIZING OSPI...");

    ospi_result = BootOspi_Init();
    if (ospi_result != BOOT_OSPI_OK)
    {
        BootDisplay_LogColor("OSPI INIT FAILED", BOOT_DISPLAY_COLOR_RED);
        printf("[OSPI] init FAILED result=%u\n", (unsigned int)ospi_result);
        return;
    }

    result = UsbFsService_GetFileSize(USB_UPDATE_APP_OSPI_BIN, &file_size);
    if (result != USB_FS_RESULT_OK)
    {
        BootDisplay_LogColor("OSPI FILE ERROR", BOOT_DISPLAY_COLOR_RED);
        printf("[OSPI] GetFileSize FAILED result=%u\n", (unsigned int)result);
        return;
    }
    printf("[OSPI] file size=%lu bytes\n", (unsigned long)file_size);

    printf("[OSPI] erasing...\n");
    BootDisplay_Log("ERASING OSPI FLASH...");
    BootDisplay_ShowProgress("ERASING OSPI FLASH...", 0U, 1U);
    ospi_result = BootOspi_Erase(file_size, BootDisplay_OspiEraseProgress);
    if (ospi_result != BOOT_OSPI_OK)
    {
        BootDisplay_LogColor("OSPI FLASH ERASE FAILED", BOOT_DISPLAY_COLOR_RED);
        BootDisplay_ClearProgress();
        printf("[OSPI] erase FAILED result=%u\n", (unsigned int)ospi_result);
        return;
    }

    printf("[OSPI] programming...\n");
    BootDisplay_Log("OSPI FLASH ERASE DONE");
    BootDisplay_Log("WRITING OSPI FLASH...");
    BootDisplay_ClearProgress();
    BootDisplay_ShowProgress("WRITING OSPI FLASH...", 0U, file_size);
    t_start = HAL_GetTick();

    while (offset < file_size)
    {
        to_read = file_size - offset;
        if (to_read > CHUNKED_READ_BLOCK_SIZE)
        {
            to_read = CHUNKED_READ_BLOCK_SIZE;
        }

        result = UsbReadChunkWithRecovery(USB_UPDATE_APP_OSPI_BIN, chunk,
                                          offset, to_read, &bytes_read, "OSPI");
        if (result != USB_FS_RESULT_OK)
        {
            BootDisplay_LogColor("OSPI READ FAILED", BOOT_DISPLAY_COLOR_RED);
            BootDisplay_ClearProgress();
            printf("[OSPI] read FAILED at offset=%lu\n", (unsigned long)offset);
            return;
        }

        ospi_result = BootOspi_Program(offset, chunk, bytes_read);
        if (ospi_result != BOOT_OSPI_OK)
        {
            BootDisplay_LogColor("OSPI FLASH WRITE FAILED", BOOT_DISPLAY_COLOR_RED);
            BootDisplay_ClearProgress();
            printf("[OSPI] program FAILED at offset=%lu result=%u\n",
                   (unsigned long)offset, (unsigned int)ospi_result);
            return;
        }

        offset += bytes_read;
        BootDisplay_UpdateProgress("WRITING OSPI FLASH...", offset, file_size, CHUNKED_READ_BLOCK_SIZE);

        printf("[OSPI] %lu / %lu bytes (%lu%%)\n",
               (unsigned long)offset,
               (unsigned long)file_size,
               (unsigned long)((offset * 100U) / file_size));

        if (offset < file_size)
        {
            HAL_Delay(CHUNKED_READ_INTER_BLOCK_DELAY_MS);
        }
    }

    uint32_t elapsed = HAL_GetTick() - t_start;

    /* Switch to memory-mapped mode and verify CRC32 */
    if (BootOspi_EnableMemoryMapped() != BOOT_OSPI_OK)
    {
        BootDisplay_LogColor("OSPI VERIFY FAILED", BOOT_DISPLAY_COLOR_RED);
        BootDisplay_ClearProgress();
        printf("[OSPI] memory-mapped FAILED, cannot verify\n");
        return;
    }

    {
        uint32_t crc_flash;
        uint32_t t_crc = HAL_GetTick();

        printf("[OSPI] verifying CRC32 over memory-mapped OSPI...\n");
        BootDisplay_Log("OSPI FLASH WRITE DONE");
        BootDisplay_Log("VERIFYING OSPI FLASH...");
        BootDisplay_ClearProgress();
        BootDisplay_ShowProgress("VERIFYING OSPI FLASH...", 0U, 1U);

        crc_flash = crc32_update(0U, (const uint8_t *)OCTOSPI1_BASE, file_size);

        /* Re-establish memory-mapped for app, then deinit CRC */
        HAL_OSPI_Abort(BootOspi_GetHandle());
        if (BootOspi_EnableMemoryMapped() != BOOT_OSPI_OK)
        {
            BootDisplay_LogColor("OSPI VERIFY FAILED", BOOT_DISPLAY_COLOR_RED);
            printf("[OSPI] re-enable memory-mapped FAILED\n");
        }
        crc32_deinit();

        printf("[OSPI] CRC32=%08lX  expected=%08lX  %s  (%lu ms)\n",
               (unsigned long)crc_flash,
               (unsigned long)expected_crc32,
               (crc_flash == expected_crc32) ? "MATCH" : "MISMATCH",
               (unsigned long)(HAL_GetTick() - t_crc));
        BootDisplay_ShowProgress("VERIFYING OSPI FLASH...", 1U, 1U);
        BootDisplay_LogColor((crc_flash == expected_crc32) ? "OSPI FLASH CRC OK" : "OSPI FLASH CRC FAIL",
                             (crc_flash == expected_crc32) ? BOOT_DISPLAY_COLOR_NORMAL : BOOT_DISPLAY_COLOR_RED);
    }

    printf("[OSPI] done in %lu ms\n", (unsigned long)elapsed);

    /* Jump to app if valid */
    if (Boot_IsApplicationValid(APP_BASE))
    {
        BootDisplay_LogColor("UPDATE COMPLETE", BOOT_DISPLAY_COLOR_GREEN);
        BootDisplay_LogColor("STARTING APPLICATION...", BOOT_DISPLAY_COLOR_GREEN);
        while (1) {} /* TEMPORARY: freeze to capture final screen */
        printf("[BOOT] app valid -> jumping...\n");
        UsbFsService_Unmount();
        UsbMscService_SetEnabled(0U);
        HAL_Delay(100U);
        Boot_JumpToApplication(APP_BASE);
    }
    else
    {
        BootDisplay_LogColor("UPDATE COMPLETE", BOOT_DISPLAY_COLOR_GREEN);
        BootDisplay_LogColor("APP INVALID", BOOT_DISPLAY_COLOR_RED);
        printf("[BOOT] app NOT valid, staying in bootloader\n");
    }
}

/* -----------------------------------------------------------------------
 * UsbUpdateLoop — Wait for USB pendrive, read update files and program.
 * ----------------------------------------------------------------------- */
static void UsbUpdateLoop(void)
{
    UsbMscState_t last_state;
    UsbFsResult_t mount_result;

    printf("[UPDATE] waiting for USB pendrive...\n");
    BootDisplay_Log("WAITING FOR USB DRIVE...");

    if (UsbMscService_IsDeviceConnected() != 0U)
    {
        BootDisplay_Log("USB DRIVE DETECTED");
        last_state = USB_MSC_STATE_DEVICE_CONNECTED;
    }
    else
    {
        last_state = UsbMscService_GetState();
    }
    printf("[UPDATE] USB state: %s\n", UsbMscService_GetStateName());

    while (1)
    {
        UsbMscService_Process();

        if (UsbMscService_GetState() != last_state)
        {
            last_state = UsbMscService_GetState();
            printf("[UPDATE] USB state: %s\n", UsbMscService_GetStateName());

            if (last_state == USB_MSC_STATE_DEVICE_CONNECTED)
            {
                BootDisplay_Log("USB DRIVE DETECTED");
            }

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

                printf("[UPDATE] MSC ready -> mounting\n");
                mount_result = UsbFsService_Mount();

                if (mount_result != USB_FS_RESULT_OK)
                {
                    BootDisplay_LogColor("USB MOUNT FAILED", BOOT_DISPLAY_COLOR_RED);
                    printf("[UPDATE] f_mount FAILED result=%u fatfs_err=%lu\n",
                           (unsigned int)mount_result,
                           (unsigned long)UsbFsService_GetLastError());
                }
                else
                {
                    printf("[UPDATE] f_mount OK\n");
                    BootDisplay_Log("USB DRIVE MOUNTED");
                    BootDisplay_Log("CHECKING UPDATE FILES...");

                    {
                        FATFS  *fs_ptr;
                        DWORD   free_clust;
                        if (f_getfree("0:", &free_clust, &fs_ptr) == FR_OK)
                        {
                            printf("[UPDATE] sector=%u bytes  cluster=%u sectors (%lu bytes)\n",
                                   (unsigned int)FF_MAX_SS,
                                   (unsigned int)fs_ptr->csize,
                                   (unsigned long)((uint32_t)fs_ptr->csize * FF_MAX_SS));
                        }
                    }

                    all_present = 1U;
                    for (i = 0U; i < 4U; i++)
                    {
                        stat_result = UsbFsService_FileExists(probe_paths[i]);
                        printf("[UPDATE] %s -> %s\n",
                               probe_paths[i],
                               (stat_result == USB_FS_RESULT_OK) ? "OK" : "NOT FOUND");
                        if (stat_result != USB_FS_RESULT_OK)
                        {
                            all_present = 0U;
                        }
                    }

                    if (all_present != 0U)
                    {
                        uint32_t fsize_tmp;
                        if (UsbFsService_GetFileSize(USB_UPDATE_APP_INT_BIN, &fsize_tmp) == USB_FS_RESULT_OK)
                            printf("[UPDATE] %s size=%lu bytes\n", USB_UPDATE_APP_INT_BIN, (unsigned long)fsize_tmp);
                        if (UsbFsService_GetFileSize(USB_UPDATE_APP_OSPI_BIN, &fsize_tmp) == USB_FS_RESULT_OK)
                            printf("[UPDATE] %s size=%lu bytes\n", USB_UPDATE_APP_OSPI_BIN, (unsigned long)fsize_tmp);

                        static const char * const crc_paths[2] =
                        {
                            USB_UPDATE_APP_INT_CRC,
                            USB_UPDATE_APP_OSPI_CRC
                        };
                        uint8_t  crc_buf[16];
                        uint32_t bytes_read;
                        UsbFsResult_t read_result;
                        uint32_t expected_crc[2] = { 0U, 0U };

                        printf("[UPDATE] all 4 files present\n");
                        BootDisplay_Log("ALL FILES FOUND");
                        BootDisplay_Log("READING CRC FILES...");

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
                                printf("[UPDATE] %s -> 0x%08lX\n",
                                       crc_paths[i], (unsigned long)val);
                            }
                            else
                            {
                                BootDisplay_LogColor("CRC FILE READ FAILED", BOOT_DISPLAY_COLOR_RED);
                                printf("[UPDATE] read %s FAILED result=%u fatfs_err=%lu\n",
                                       crc_paths[i],
                                       (unsigned int)read_result,
                                       (unsigned long)UsbFsService_GetLastError());
                            }
                        }

                        /* Program both flashes and jump */
                        FlashAppInt(expected_crc[0]);
                        FlashAppOspi(expected_crc[1]);
                    }
                    else
                    {
                        BootDisplay_LogColor("UPDATE FILES MISSING", BOOT_DISPLAY_COLOR_RED);
                        printf("[UPDATE] one or more files missing\n");
                    }
                }
            }
            else if (last_state == USB_MSC_STATE_IDLE)
            {
                printf("[UPDATE] device disconnected -> unmounting\n");
                UsbFsService_Unmount();
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * SystemPower_Config
 * ----------------------------------------------------------------------- */
static void SystemPower_Config(void)
{
    if (HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -----------------------------------------------------------------------
 * SystemClock_Config
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
 * ----------------------------------------------------------------------- */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = FS_PW_SW_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(FS_PW_SW_GPIO_Port, &GPIO_InitStruct);

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
