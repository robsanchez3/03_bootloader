/**
  * @file   main.c
  * @brief  O3 Bootloader
  *
  *  Behaviour:
  *    1. Minimal HW init (clocks, GPIO).
  *    2. Perform a silent USB boot check at power-on.
  *    3. If a ready update drive is found, run the update flow.
  *    4. Otherwise jump to the application if valid, or enter recovery.
  */

#include "stm32u5xx_hal.h"
#include "main.h"
#include "boot_jump.h"
#include "boot_flash.h"
#include "boot_ospi.h"
#include "boot_display.h"
#include "boot_crc.h"
#include "boot_manifest.h"
#include "usb_msc_service.h"
#include "usb_fs_service.h"
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
static void Boot_SelectBootPath(void);
static void Boot_SelectAppOrRecovery(void);
static void FlashAppInt(uint32_t expected_crc32);
static void FlashAppOspi(uint32_t expected_crc32,
                         uint32_t int_crc32, uint32_t int_size,
                         uint32_t ospi_size);
static void UsbProcessUpdate(void);
static uint8_t UsbBootCheck(uint32_t detect_timeout_ms, uint32_t ready_timeout_ms);
static void BootDisplay_EnsureInit(void);
static void Boot_ShowCountdown(uint32_t row0,
                               const char *line0,
                               uint32_t row1,
                               const char *line1,
                               uint32_t row2,
                               BootDisplayColor_t color);
static void Boot_ShowApplicationLaunchCountdown(void);
static void Boot_ShowProgrammingCountdown(const char *ver_line, const char *build_date);
static void BootDisplay_UpdateProgress(const char *label, uint32_t current, uint32_t total);
static void BootDisplay_Fail(const char *message, uint8_t clear_progress);
static void BootDisplay_FlashEraseProgress(uint32_t current, uint32_t total);
static void BootDisplay_OspiEraseProgress(uint32_t current, uint32_t total);

#define BOOT_FORCE_INVALID_APP_FOR_TEST   0U
#define BOOT_USB_UPDATE_ENABLED           1U
#define BOOT_PRE_FLASH_CRC_CHECK         1U
#define BOOT_USB_DETECT_TIMEOUT_MS        1800U
#define BOOT_USB_READY_TIMEOUT_MS         2500U

#define USB_UPDATE_DIR_PATH               "0:/UPDATE"
#define USB_UPDATE_APP_INT_BIN            "0:/UPDATE/app_int.bin"
#define USB_UPDATE_APP_OSPI_BIN           "0:/UPDATE/app_ospi.bin"
#define USB_UPDATE_MANIFEST               "0:/UPDATE/manifest.ini"

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


static void BootDisplay_EnsureInit(void)
{
    if (boot_display_initialized == 0U)
    {
        BootDisplay_Init();
        boot_display_initialized = 1U;
    }
}

/* -----------------------------------------------------------------------
 * UsbBootCheck — Silent power-on USB check used to decide whether the
 *                bootloader update flow should run.
 * ----------------------------------------------------------------------- */
static uint8_t UsbBootCheck(uint32_t detect_timeout_ms, uint32_t ready_timeout_ms)
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

static void BootDisplay_UpdateProgress(const char *label, uint32_t current, uint32_t total)
{
    uint32_t shown;

    if (total == 0U)
    {
        BootDisplay_ShowProgress(label, 0U, 1U);
        return;
    }

    shown = current;
    if (shown > total)
    {
        shown = total;
    }

    BootDisplay_ShowProgress(label, shown, total);
}

static void BootDisplay_Fail(const char *message, uint8_t clear_progress)
{
    BootDisplay_LogColor(message, BOOT_DISPLAY_COLOR_RED);
    if (clear_progress != 0U)
    {
        BootDisplay_ClearProgress();
    }
}

static void Boot_ShowCountdown(uint32_t row0,
                               const char *line0,
                               uint32_t row1,
                               const char *line1,
                               uint32_t row2,
                               BootDisplayColor_t color)
{
    uint32_t elapsed;
    uint32_t duration = 30U;
    char line[32];
    char bar[11];
    uint32_t i;

    BootDisplay_ClearLine(row0);
    BootDisplay_ClearLine(row1);
    BootDisplay_ClearLine(row2);
    BootDisplay_WriteLineColor(row0, line0, color);
    if ((line1 != NULL) && (line1[0] != '\0'))
    {
        BootDisplay_WriteLineColor(row1, line1, color);
    }

    for (elapsed = 0U; elapsed <= duration; elapsed++)
    {
        uint32_t remaining = duration - elapsed;
        uint32_t percent = (elapsed * 100U) / duration;
        uint32_t filled = 10U - (percent / 10U);

        for (i = 0U; i < 10U; i++)
        {
            bar[i] = (i < filled) ? '#' : '.';
        }
        bar[10] = '\0';

        (void)snprintf(line, sizeof(line), "[%s] %2lus",
                       bar,
                       (unsigned long)remaining);
        BootDisplay_WriteLineColor(row2, line, color);

        if (elapsed < duration)
        {
            HAL_Delay(1000U);
        }
    }
}

static void Boot_ShowApplicationLaunchCountdown(void)
{
    BootDisplay_LogColor("UPDATE COMPLETE", BOOT_DISPLAY_COLOR_GREEN);
    BootDisplay_ClearProgress();
    Boot_ShowCountdown(28U,
                       "STARTING APPLICATION...",
                       29U,
                       "",
                       29U,
                       BOOT_DISPLAY_COLOR_YELLOW);

    BootDisplay_ClearProgress();
    BootDisplay_LogColor("STARTING APPLICATION...", BOOT_DISPLAY_COLOR_GREEN);
    while (1) {} /* TEMPORARY: keep final bootloader screen visible for tests */
}

static void Boot_ShowProgrammingCountdown(const char *ver_line, const char *build_date)
{
    BootDisplay_ClearLine(26U);
    BootDisplay_WriteLineColor(26U, ver_line, BOOT_DISPLAY_COLOR_YELLOW);
    BootDisplay_ClearLine(27U);
    BootDisplay_WriteLineColor(27U, build_date, BOOT_DISPLAY_COLOR_YELLOW);
    Boot_ShowCountdown(25U,
                       "PROGRAMMING STARTS IN 30s",
                       28U,
                       "POWER OFF & REMOVE USB TO SKIP",
                       29U,
                       BOOT_DISPLAY_COLOR_YELLOW);
    BootDisplay_ClearLine(25U);
    BootDisplay_ClearLine(26U);
    BootDisplay_ClearLine(27U);
    BootDisplay_ClearLine(28U);
    BootDisplay_ClearLine(29U);
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
    Boot_SelectBootPath();

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
    BootDisplay_LogColor("NO VALID APP", BOOT_DISPLAY_COLOR_RED);
    BootDisplay_Log("");
    BootDisplay_LogColor("INSERT UPDATE USB", BOOT_DISPLAY_COLOR_YELLOW);
    BootDisplay_LogColor("AND REBOOT", BOOT_DISPLAY_COLOR_YELLOW);

    while (1)
    {
        if ((HAL_GetTick() - last_blink) >= 250U)
        {
            last_blink = HAL_GetTick();
            HAL_GPIO_TogglePin(BOOT_LED_GPIO_Port, BOOT_LED_Pin);
        }
    }
}

/* -----------------------------------------------------------------------
 * Boot_SelectBootPath
 * ----------------------------------------------------------------------- */
static void Boot_SelectBootPath(void)
{
#if BOOT_USB_UPDATE_ENABLED
    if (UsbBootCheck(BOOT_USB_DETECT_TIMEOUT_MS, BOOT_USB_READY_TIMEOUT_MS) != 0U)
    {
        BootDisplay_EnsureInit();
        BootDisplay_LogColor("BOOT START", BOOT_DISPLAY_COLOR_BLUE);
        UsbProcessUpdate();
        return;
    }
#endif

    Boot_SelectAppOrRecovery();
}

/* -----------------------------------------------------------------------
 * Boot_SelectAppOrRecovery
 * ----------------------------------------------------------------------- */
static void Boot_SelectAppOrRecovery(void)
{
    uint8_t app_valid = (uint8_t)Boot_IsApplicationValid(APP_BASE);

#if BOOT_FORCE_INVALID_APP_FOR_TEST
    printf("[BOOT] TEST MODE: forcing application invalid\n");
    app_valid = 0U;
#endif

    if (app_valid != 0U)
    {
        if (Boot_VerifyAppCrc() == 0)
        {
            app_valid = 0U;
        }
    }

    if (app_valid != 0U)
    {
        UsbFsService_Unmount();
        UsbMscService_SetEnabled(0U);
        HAL_Delay(50U);
        Boot_JumpToApplication(APP_BASE);
    }

    BootDisplay_EnsureInit();
    printf("[BOOT] Recovery mode\n");
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

    /* Preparation: resolve input file size before touching flash. */
    result = UsbFsService_GetFileSize(USB_UPDATE_APP_INT_BIN, &file_size);
    if (result != USB_FS_RESULT_OK)
    {
        BootDisplay_Fail("INT FLASH FILE ERROR", 0U);
        printf("[FLASH] GetFileSize FAILED result=%u\n", (unsigned int)result);
        return;
    }
    printf("[FLASH] file size=%lu bytes\n", (unsigned long)file_size);

    /* Erase phase. */
    printf("[FLASH] erasing...\n");
    BootDisplay_ShowProgress("ERASING INT FLASH...", 0U, 1U);
    flash_result = BootFlash_EraseAppArea(file_size, BootDisplay_FlashEraseProgress);
    if (flash_result != BOOT_FLASH_OK)
    {
        BootDisplay_Fail("INT FLASH ERASE FAILED", 1U);
        printf("[FLASH] erase FAILED result=%u\n", (unsigned int)flash_result);
        return;
    }

    /* Write phase. */
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
            BootDisplay_Fail("INT FLASH READ FAILED", 1U);
            printf("[FLASH] read FAILED at offset=%lu\n", (unsigned long)offset);
            return;
        }

        /* Program and verify in 64KB sub-blocks so the display can refresh. */
        {
            uint32_t sub_off = 0U;
            while (sub_off < bytes_read)
            {
                uint32_t sub_len = bytes_read - sub_off;
                if (sub_len > 65536U)
                {
                    sub_len = 65536U;
                }

                flash_result = BootFlash_Program(APP_BASE + offset + sub_off,
                                                 &chunk[sub_off], sub_len);
                if (flash_result != BOOT_FLASH_OK)
                {
                    BootDisplay_Fail("INT FLASH WRITE FAILED", 1U);
                    printf("[FLASH] program FAILED at offset=%lu result=%u\n",
                           (unsigned long)(offset + sub_off), (unsigned int)flash_result);
                    return;
                }

                flash_result = BootFlash_Verify(APP_BASE + offset + sub_off,
                                                &chunk[sub_off], sub_len);
                if (flash_result != BOOT_FLASH_OK)
                {
                    BootDisplay_Fail("INT FLASH VERIFY FAILED", 1U);
                    printf("[FLASH] verify FAILED at offset=%lu\n",
                           (unsigned long)(offset + sub_off));
                    return;
                }

                sub_off += sub_len;
                BootDisplay_UpdateProgress("WRITING INT FLASH...",
                                           offset + sub_off, file_size);
                HAL_Delay(20U);
            }
        }

        offset += bytes_read;

        printf("[FLASH] %lu / %lu bytes (%lu%%)\n",
               (unsigned long)offset,
               (unsigned long)file_size,
               (unsigned long)((offset * 100U) / file_size));

        if (offset < file_size)
        {
            HAL_Delay(CHUNKED_READ_INTER_BLOCK_DELAY_MS);
        }
    }

    /* Verify phase. */
    BootDisplay_Log("INT FLASH WRITE DONE");
    BootDisplay_Log("VERIFYING INT FLASH...");
    BootDisplay_ClearProgress();
    {
        uint32_t crc_flash = BootCrc32_Compute(0U, (const uint8_t *)APP_BASE, file_size);
        BootDisplay_LogColor((crc_flash == expected_crc32) ? "INT FLASH CRC OK" : "INT FLASH CRC FAIL",
                             (crc_flash == expected_crc32) ? BOOT_DISPLAY_COLOR_NORMAL : BOOT_DISPLAY_COLOR_RED);

        printf("[FLASH] CRC32=%08lX  expected=%08lX  %s\n",
               (unsigned long)crc_flash,
               (unsigned long)expected_crc32,
               (crc_flash == expected_crc32) ? "MATCH" : "MISMATCH");
    }

    /* Final state. */
    printf("[FLASH] Boot_IsApplicationValid: %s\n",
           Boot_IsApplicationValid(APP_BASE) ? "VALID" : "INVALID");

    printf("[FLASH] done in %lu ms\n", (unsigned long)(HAL_GetTick() - t_start));
}

/* -----------------------------------------------------------------------
 * FlashAppOspi — Read app_ospi.bin from USB, program OSPI flash, verify,
 *                re-enable memory-mapped mode and decide final app state.
 * ----------------------------------------------------------------------- */
static void FlashAppOspi(uint32_t expected_crc32,
                         uint32_t int_crc32, uint32_t int_size,
                         uint32_t ospi_size)
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

    /* Preparation: initialize OSPI and resolve input file size. */
    ospi_result = BootOspi_Init();
    if (ospi_result != BOOT_OSPI_OK)
    {
        BootDisplay_Fail("OSPI INIT FAILED", 0U);
        printf("[OSPI] init FAILED result=%u\n", (unsigned int)ospi_result);
        return;
    }

    result = UsbFsService_GetFileSize(USB_UPDATE_APP_OSPI_BIN, &file_size);
    if (result != USB_FS_RESULT_OK)
    {
        BootDisplay_Fail("OSPI FILE ERROR", 0U);
        printf("[OSPI] GetFileSize FAILED result=%u\n", (unsigned int)result);
        return;
    }
    printf("[OSPI] file size=%lu bytes\n", (unsigned long)file_size);

    /* Erase phase. */
    printf("[OSPI] erasing...\n");
    BootDisplay_ShowProgress("ERASING OSPI FLASH...", 0U, 1U);
    ospi_result = BootOspi_Erase(file_size, BootDisplay_OspiEraseProgress);
    if (ospi_result != BOOT_OSPI_OK)
    {
        BootDisplay_Fail("OSPI FLASH ERASE FAILED", 1U);
        printf("[OSPI] erase FAILED result=%u\n", (unsigned int)ospi_result);
        return;
    }

    /* Write phase. */
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
            BootDisplay_Fail("OSPI READ FAILED", 1U);
            printf("[OSPI] read FAILED at offset=%lu\n", (unsigned long)offset);
            return;
        }

        ospi_result = BootOspi_Program(offset, chunk, bytes_read);
        if (ospi_result != BOOT_OSPI_OK)
        {
            BootDisplay_Fail("OSPI FLASH WRITE FAILED", 1U);
            printf("[OSPI] program FAILED at offset=%lu result=%u\n",
                   (unsigned long)offset, (unsigned int)ospi_result);
            return;
        }

        offset += bytes_read;
        BootDisplay_UpdateProgress("WRITING OSPI FLASH...", offset, file_size);

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

    /* Verify phase: switch to memory-mapped mode and compute CRC32. */
    if (BootOspi_EnableMemoryMapped() != BOOT_OSPI_OK)
    {
        BootDisplay_Fail("OSPI VERIFY FAILED", 1U);
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
        BootDisplay_ShowProgress("VERIFYING OSPI FLASH...", 0U, file_size);

        /* CRC could be computed in one call, but chunking in 64KB lets
         * the LTDC refresh the progress bar between iterations. */
        {
            uint32_t crc_off = 0U;
            crc_flash = 0U;
            while (crc_off < file_size)
            {
                uint32_t crc_len = file_size - crc_off;
                if (crc_len > 65536U)
                {
                    crc_len = 65536U;
                }
                crc_flash = BootCrc32_Compute(crc_flash,
                                              (const uint8_t *)(OCTOSPI1_BASE + crc_off),
                                              crc_len);
                crc_off += crc_len;
                BootDisplay_UpdateProgress("VERIFYING OSPI FLASH...",
                                           crc_off, file_size);
                HAL_Delay(20U);
            }
        }

        /* Re-establish memory-mapped for app, then deinit CRC */
        HAL_OSPI_Abort(BootOspi_GetHandle());
        if (BootOspi_EnableMemoryMapped() != BOOT_OSPI_OK)
        {
            BootDisplay_Fail("OSPI VERIFY FAILED", 0U);
            printf("[OSPI] re-enable memory-mapped FAILED\n");
        }
        BootCrc32_DeInit();

        printf("[OSPI] CRC32=%08lX  expected=%08lX  %s  (%lu ms)\n",
               (unsigned long)crc_flash,
               (unsigned long)expected_crc32,
               (crc_flash == expected_crc32) ? "MATCH" : "MISMATCH",
               (unsigned long)(HAL_GetTick() - t_crc));
        BootDisplay_ShowProgress("VERIFYING OSPI FLASH...", 1U, 1U);
        BootDisplay_LogColor((crc_flash == expected_crc32) ? "OSPI FLASH CRC OK" : "OSPI FLASH CRC FAIL",
                             (crc_flash == expected_crc32) ? BOOT_DISPLAY_COLOR_NORMAL : BOOT_DISPLAY_COLOR_RED);
    }

    /* Final state. */
    printf("[OSPI] done in %lu ms\n", (unsigned long)elapsed);

    /* Final state: jump only if the resulting application is valid. */
    if (Boot_IsApplicationValid(APP_BASE))
    {
        Boot_StoreAppCrc(int_crc32, int_size, expected_crc32, ospi_size);
        Boot_ShowApplicationLaunchCountdown();
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
 * UsbProcessUpdate — Process the update flow for a USB drive that is already
 *                    connected and ready after the silent boot check.
 * ----------------------------------------------------------------------- */
static void UsbProcessUpdate(void)
{
    UsbFsResult_t mount_result;
    static const char * const update_paths[3] =
    {
        USB_UPDATE_APP_INT_BIN,
        USB_UPDATE_APP_OSPI_BIN,
        USB_UPDATE_MANIFEST
    };
    uint32_t i;
    UsbFsResult_t stat_result;
    uint8_t all_present;
    uint32_t fsize_tmp;
    BootManifest_t manifest;
    BootManifestResult_t mresult;
    char ver_line[48];

    printf("[UPDATE] update drive accepted by boot check\n");
    BootDisplay_Log("WAITING FOR USB DRIVE...");
    BootDisplay_Log("USB DRIVE DETECTED");
    printf("[UPDATE] USB state: %s\n", UsbMscService_GetStateName());
    printf("[UPDATE] MSC ready -> mounting\n");
    mount_result = UsbFsService_Mount();

    if (mount_result != USB_FS_RESULT_OK)
    {
        BootDisplay_Fail("USB MOUNT FAILED", 0U);
        printf("[UPDATE] f_mount FAILED result=%u fatfs_err=%lu\n",
               (unsigned int)mount_result,
               (unsigned long)UsbFsService_GetLastError());
        return;
    }

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
    for (i = 0U; i < 3U; i++)
    {
        stat_result = UsbFsService_FileExists(update_paths[i]);
        printf("[UPDATE] %s -> %s\n",
               update_paths[i],
               (stat_result == USB_FS_RESULT_OK) ? "OK" : "NOT FOUND");
        if (stat_result != USB_FS_RESULT_OK)
        {
            all_present = 0U;
        }
    }

    if (all_present == 0U)
    {
        BootDisplay_Fail("UPDATE FILES MISSING", 0U);
        printf("[UPDATE] one or more files missing\n");
        return;
    }

    printf("[UPDATE] all files present\n");
    BootDisplay_Log("ALL FILES FOUND");
    BootDisplay_Log("READING MANIFEST...");

    /* Parse manifest and validate its integrity CRC */
    mresult = BootManifest_LoadAndParse(USB_UPDATE_MANIFEST, &manifest);
    if (mresult != BOOT_MANIFEST_OK)
    {
        const char *err_msg = "MANIFEST ERROR";
        if (mresult == BOOT_MANIFEST_ERR_INTEGRITY)
            err_msg = "MANIFEST CRC FAILED";
        else if (mresult == BOOT_MANIFEST_ERR_PARSE)
            err_msg = "MANIFEST PARSE FAILED";
        else if (mresult == BOOT_MANIFEST_ERR_READ)
            err_msg = "MANIFEST READ FAILED";

        BootDisplay_Fail(err_msg, 0U);
        printf("[UPDATE] manifest error=%u\n", (unsigned int)mresult);
        return;
    }

    BootManifest_Print(&manifest);
    BootDisplay_Log("MANIFEST OK");

    /* Display version info — strip trailing _x suffix (e.g. V1.R1.P1_b -> V1.R1.P1) */
    {
        char sw_short[20];
        char lib_short[20];
        char *p;

        strncpy(sw_short, manifest.sw_version, sizeof(sw_short) - 1U);
        sw_short[sizeof(sw_short) - 1U] = '\0';
        p = strrchr(sw_short, '_');
        if (p != NULL) *p = '\0';

        strncpy(lib_short, manifest.o3_lib_version, sizeof(lib_short) - 1U);
        lib_short[sizeof(lib_short) - 1U] = '\0';
        p = strrchr(lib_short, '_');
        if (p != NULL) *p = '\0';

        {
            char log_line[32];
            (void)snprintf(log_line, sizeof(log_line), "SW: %s", sw_short);
            BootDisplay_LogColor(log_line, BOOT_DISPLAY_COLOR_BLUE);
            (void)snprintf(log_line, sizeof(log_line), "LIB: %s", lib_short);
            BootDisplay_LogColor(log_line, BOOT_DISPLAY_COLOR_BLUE);
        }

        (void)snprintf(ver_line, sizeof(ver_line), "SW: %s - LIB: %s",
                       sw_short, lib_short);
    }

    /* Validate file sizes against manifest */
    if (UsbFsService_GetFileSize(USB_UPDATE_APP_INT_BIN, &fsize_tmp) == USB_FS_RESULT_OK)
    {
        printf("[UPDATE] %s size=%lu bytes\n", USB_UPDATE_APP_INT_BIN, (unsigned long)fsize_tmp);
        if (fsize_tmp != manifest.app_int.size)
        {
            BootDisplay_Fail("INT FILE SIZE MISMATCH", 0U);
            printf("[UPDATE] expected=%lu actual=%lu\n",
                   (unsigned long)manifest.app_int.size, (unsigned long)fsize_tmp);
            return;
        }
    }
    if (UsbFsService_GetFileSize(USB_UPDATE_APP_OSPI_BIN, &fsize_tmp) == USB_FS_RESULT_OK)
    {
        printf("[UPDATE] %s size=%lu bytes\n", USB_UPDATE_APP_OSPI_BIN, (unsigned long)fsize_tmp);
        if (fsize_tmp != manifest.app_ospi.size)
        {
            BootDisplay_Fail("OSPI FILE SIZE MISMATCH", 0U);
            printf("[UPDATE] expected=%lu actual=%lu\n",
                   (unsigned long)manifest.app_ospi.size, (unsigned long)fsize_tmp);
            return;
        }
    }

#if BOOT_PRE_FLASH_CRC_CHECK
    /* Pre-flash CRC check of app_int.bin — read from USB and verify before
     * touching any flash.  app_ospi.bin is too large to pre-verify. */
    BootDisplay_Log("VERIFYING INT BIN CRC...");
    {
        uint32_t pre_crc = 0U;
        uint32_t pre_off = 0U;
        uint32_t pre_read = 0U;

        while (pre_off < manifest.app_int.size)
        {
            uint32_t pre_len = manifest.app_int.size - pre_off;
            if (pre_len > CHUNKED_READ_BLOCK_SIZE)
            {
                pre_len = CHUNKED_READ_BLOCK_SIZE;
            }
            if (UsbReadChunkWithRecovery(USB_UPDATE_APP_INT_BIN, io_buf,
                                         pre_off, pre_len, &pre_read,
                                         "PRE-CRC") != USB_FS_RESULT_OK)
            {
                BootDisplay_Fail("INT BIN READ FAILED", 0U);
                printf("[UPDATE] pre-CRC read FAILED at offset=%lu\n",
                       (unsigned long)pre_off);
                return;
            }
            pre_crc = BootCrc32_Compute(pre_crc, io_buf, pre_read);
            pre_off += pre_read;
        }

        if (pre_crc != manifest.app_int.crc32)
        {
            BootDisplay_Fail("INT BIN CRC MISMATCH", 0U);
            printf("[UPDATE] app_int.bin CRC=%08lX expected=%08lX\n",
                   (unsigned long)pre_crc,
                   (unsigned long)manifest.app_int.crc32);
            return;
        }
        printf("[UPDATE] app_int.bin pre-CRC OK\n");
        BootDisplay_Log("INT BIN CRC OK");
    }

    /* Pre-flash CRC check of app_ospi.bin — same approach, slower (~140s). */
    BootDisplay_Log("VERIFYING OSPI BIN CRC...");
    BootDisplay_ShowProgress("VERIFYING OSPI BIN CRC...", 0U, manifest.app_ospi.size);
    {
        uint32_t pre_crc = 0U;
        uint32_t pre_off = 0U;
        uint32_t pre_read = 0U;

        while (pre_off < manifest.app_ospi.size)
        {
            uint32_t pre_len = manifest.app_ospi.size - pre_off;
            if (pre_len > CHUNKED_READ_BLOCK_SIZE)
            {
                pre_len = CHUNKED_READ_BLOCK_SIZE;
            }
            if (UsbReadChunkWithRecovery(USB_UPDATE_APP_OSPI_BIN, io_buf,
                                         pre_off, pre_len, &pre_read,
                                         "PRE-CRC") != USB_FS_RESULT_OK)
            {
                BootDisplay_Fail("OSPI BIN READ FAILED", 1U);
                printf("[UPDATE] pre-CRC read FAILED at offset=%lu\n",
                       (unsigned long)pre_off);
                return;
            }
            pre_crc = BootCrc32_Compute(pre_crc, io_buf, pre_read);
            pre_off += pre_read;
            BootDisplay_UpdateProgress("VERIFYING OSPI BIN CRC...",
                                       pre_off, manifest.app_ospi.size);

            if (pre_off < manifest.app_ospi.size)
            {
                HAL_Delay(CHUNKED_READ_INTER_BLOCK_DELAY_MS);
            }
        }

        if (pre_crc != manifest.app_ospi.crc32)
        {
            BootDisplay_Fail("OSPI BIN CRC MISMATCH", 1U);
            printf("[UPDATE] app_ospi.bin CRC=%08lX expected=%08lX\n",
                   (unsigned long)pre_crc,
                   (unsigned long)manifest.app_ospi.crc32);
            return;
        }
        printf("[UPDATE] app_ospi.bin pre-CRC OK\n");
        BootDisplay_Log("OSPI BIN CRC OK");
        BootDisplay_ClearProgress();
    }
#endif

    {
        char date_upper[32];
        uint32_t di;

        strncpy(date_upper, manifest.build_date, sizeof(date_upper) - 1U);
        date_upper[sizeof(date_upper) - 1U] = '\0';
        for (di = 0U; date_upper[di] != '\0'; di++)
        {
            if ((date_upper[di] >= 'a') && (date_upper[di] <= 'z'))
            {
                date_upper[di] = date_upper[di] - ('a' - 'A');
            }
        }
        Boot_ShowProgrammingCountdown(ver_line, date_upper);
    }

    /* Invalidate stored CRCs before touching flash.  This ensures that if
     * power is lost at any point during programming, the next boot will see
     * a valid magic but mismatched CRCs and enter recovery mode.
     * The correct CRCs are written at the end of FlashAppOspi on success.
     * This keeps a single path to recovery (CRC mismatch) regardless of
     * whether the failure is a mid-update power loss or post-update
     * corruption. */
    Boot_StoreAppCrc(0U, 0U, 0U, 0U);

    FlashAppInt(manifest.app_int.crc32);
    FlashAppOspi(manifest.app_ospi.crc32,
                 manifest.app_int.crc32, manifest.app_int.size,
                 manifest.app_ospi.size);
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
