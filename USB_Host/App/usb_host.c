/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file            : USB_Host/App/usb_host.c
  * @brief           : USB Host application layer - STM32U5 adaptation
  *
  * Adapted from OztDUI_UsbFlash (STM32F7) reference project.
  * Key changes vs reference:
  *   - HOST_FS  -> HOST_HS  (USB_OTG_HS peripheral)
  *   - hUsbHostFS -> hUsbHostHS
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"
#include "fatfs_usb.h"
#include "boot_jump.h"
#include <stdio.h>
#include <stdlib.h>
	
/* USER CODE BEGIN Includes */
#define USB_HOST_DEBUG
#ifdef USB_HOST_DEBUG
#define USB_LOG(...)  printf(__VA_ARGS__)
#else
#define USB_LOG(...)  do {} while(0)
#endif

#define USB_UPDATE_INT_BIN_PATH      "0:/UPDATE/app_int.bin"
#define USB_UPDATE_INT_CRC_PATH      "0:/UPDATE/app_int.crc"
#define USB_UPDATE_OSPI_BIN_PATH     "0:/UPDATE/app_ospi.bin"
#define USB_UPDATE_OSPI_CRC_PATH     "0:/UPDATE/app_ospi.crc"
#define USB_OSPI_SIZE_BYTES          (32U * 1024U * 1024U)
#define USB_ENUM_TIMEOUT_MS         10000U
#define USB_MSC_INIT_TIMEOUT_MS     15000U
#define USB_MOUNT_SETTLE_DELAY_MS    3000U
#define USB_POST_MOUNT_DELAY_MS      1000U
#define USB_RETRY_DELAY_MS            200U
#define USB_POWER_CYCLE_OFF_MS       1500U
#define USB_POWER_CYCLE_MAX_COUNT       3U
#define USB_FINAL_RESET_DELAY_MS     1000U
#define USB_MOUNT_RETRY_LIMIT           3U
#define USB_READ_RETRY_LIMIT            3U
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
USBH_HandleTypeDef  hUsbHostHS;
ApplicationTypeDef  Appli_state = APPLICATION_IDLE;

/* USER CODE BEGIN PV */
static uint8_t usb_update_check_pending;
static uint8_t usb_update_check_done;
static uint32_t usb_connect_tick;
static uint32_t usb_action_due_tick;
static uint32_t usb_msc_init_deadline_tick;
static uint8_t usb_reenum_attempted;
static uint8_t usb_media_recovery_attempted;
static uint8_t usb_mount_pending;
static uint8_t usb_mount_retry_count;
static uint8_t usb_read_retry_count;
static uint8_t usb_power_cycle_count;
static uint8_t usb_seen_since_clear;
static uint8_t usb_physical_connection_seen;
static uint8_t usb_valid_update_found;
static uint32_t usb_last_failure_reason_code;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/* USER CODE BEGIN PFP */
static uint8_t USBH_AppIsTransientFatFsError(FRESULT res);
static uint8_t USBH_AppIsLunReady(uint8_t lun);
static uint8_t USBH_AppTimeReached(uint32_t due_tick);
static void USBH_AppPowerCycle(uint32_t reason_code);
static uint8_t USBH_AppValidateUpdateImage(void);
static uint8_t USBH_AppValidateOneImage(const char *bin_path,
                                        const char *crc_path,
                                        uint32_t max_size_bytes,
                                        const char *label);

/* USER CODE END PFP */

/**
  * @brief  Initialize USB Host library, register MSC class and start
  * @retval None
  */
void MX_USB_HOST_Init(void)
{
  /* USER CODE BEGIN USB_HOST_Init_PreTreatment */

  /* USER CODE END USB_HOST_Init_PreTreatment */

  USB_LOG("[USB] USBH_Init...\n");
  if (USBH_Init(&hUsbHostHS, USBH_UserProcess, HOST_HS) != USBH_OK)
  {
    printf("[USB] USBH_Init FAILED\n");
    Error_Handler();
  }
  USB_LOG("[USB] USBH_RegisterClass...\n");
  if (USBH_RegisterClass(&hUsbHostHS, USBH_MSC_CLASS) != USBH_OK)
  {
    printf("[USB] USBH_RegisterClass FAILED\n");
    Error_Handler();
  }
  USB_LOG("[USB] USBH_Start...\n");
  if (USBH_Start(&hUsbHostHS) != USBH_OK)
  {
    printf("[USB] USBH_Start FAILED\n");
    Error_Handler();
  }
  USB_LOG("[USB] Init complete\n");

  /* USER CODE BEGIN USB_HOST_Init_PostTreatment */

  /* USER CODE END USB_HOST_Init_PostTreatment */
}

/**
  * @brief  Returns 1 if a USB flash drive is connected and MSC class is active.
  * @retval 1 if ready, 0 otherwise
  */
uint8_t USBH_IsFlashReady(void)
{
  return (Appli_state == APPLICATION_READY) ? 1 : 0;
}

uint8_t USBH_WasUsbSeen(void)
{
  return usb_seen_since_clear;
}

uint8_t USBH_HasPhysicalConnection(void)
{
  return usb_physical_connection_seen;
}

uint8_t USBH_HasValidUpdateImage(void)
{
  return usb_valid_update_found;
}

uint8_t USBH_IsUpdateCheckComplete(void)
{
  return usb_update_check_done;
}

void USBH_ClearUpdateDetection(void)
{
  usb_seen_since_clear = 0U;
  usb_physical_connection_seen = 0U;
  usb_valid_update_found = 0U;
}

/**
  * @brief  USB Host background task - call periodically from FreeRTOS task
  * @retval None
  */
void MX_USB_HOST_Process(void)
{
  USBH_Process(&hUsbHostHS);

  if ((Appli_state == APPLICATION_START) &&
      (usb_media_recovery_attempted == 0U) &&
      (USBH_AppTimeReached(usb_msc_init_deadline_tick) != 0U))
  {
    USBH_AppPowerCycle(2U);
    return;
  }

#if 0
  if ((Appli_state == APPLICATION_START) &&
      (usb_reenum_attempted == 0U) &&
      ((HAL_GetTick() - usb_connect_tick) >= USB_ENUM_TIMEOUT_MS))
  {
    USB_LOG("[USB] enum timeout -> re-enumerate\n");
    usb_reenum_attempted = 1U;
    (void)USBH_ReEnumerate(&hUsbHostHS);
    usb_connect_tick = HAL_GetTick();
  }
#endif
}

void USBH_AppTask(void)
{
  FRESULT mount_res;
  FRESULT read_file_res;

  if ((usb_update_check_pending == 0U) || (usb_update_check_done != 0U))
  {
    return;
  }

  if (Appli_state != APPLICATION_READY)
  {
    return;
  }

  if (USBH_AppIsLunReady(0U) == 0U)
  {
    return;
  }

  if (USBH_AppTimeReached(usb_action_due_tick) == 0U)
  {
    return;
  }

  if (usb_mount_pending != 0U)
  {
    mount_res = USB_FATFS_Mount();
    if (mount_res == FR_OK)
    {
      usb_mount_pending = 0U;
      usb_mount_retry_count = 0U;
      usb_action_due_tick = HAL_GetTick() + USB_POST_MOUNT_DELAY_MS;
      return;
    }

    if ((USBH_AppIsTransientFatFsError(mount_res) != 0U) &&
        (usb_mount_retry_count < USB_MOUNT_RETRY_LIMIT))
    {
      usb_mount_retry_count++;
      usb_action_due_tick = HAL_GetTick() + USB_RETRY_DELAY_MS;
      USB_LOG("[USB] mount retry %u scheduled\n", (unsigned)usb_mount_retry_count);
      return;
    }

    USBH_AppPowerCycle(3U);
    return;
  }

  USB_LOG("[USB] validating update image\n");
  read_file_res = (USBH_AppValidateUpdateImage() != 0U) ? FR_OK : FR_INT_ERR;
  USB_LOG("[USB] update validation result = %d\n", read_file_res);

  if (read_file_res == FR_OK)
  {
    usb_read_retry_count = 0U;
    usb_power_cycle_count = 0U;
    usb_last_failure_reason_code = 0U;
    usb_valid_update_found = 1U;
    usb_update_check_done = 1U;
    return;
  }

  if ((USBH_AppIsTransientFatFsError(read_file_res) != 0U) &&
      (usb_read_retry_count < USB_READ_RETRY_LIMIT))
  {
    usb_read_retry_count++;
    usb_action_due_tick = HAL_GetTick() + USB_RETRY_DELAY_MS;
    USB_LOG("[USB] read retry %u scheduled\n", (unsigned)usb_read_retry_count);
    return;
  }

  if ((USBH_AppIsTransientFatFsError(read_file_res) != 0U) &&
      (usb_media_recovery_attempted == 0U))
  {
    USBH_AppPowerCycle(4U);
    return;
  }

  usb_update_check_done = 1U;
}

/**
  * @brief  User callback - notified on USB Host state changes
  * @param  phost: Host handle
  * @param  id:    Event ID (HOST_USER_*)
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  FRESULT mount_res;

  (void)phost;

  /* USER CODE BEGIN CALL_BACK_1 */
  switch (id)
  {
    case HOST_USER_SELECT_CONFIGURATION:
      USB_LOG("[USB] HOST_USER_SELECT_CONFIGURATION\n");
      break;

    case HOST_USER_CONNECTION:
      USB_LOG("[USB] HOST_USER_CONNECTION -> START\n");
      usb_seen_since_clear = 1U;
      usb_physical_connection_seen = 1U;
      usb_update_check_pending = 0U;
      usb_update_check_done = 0U;
      usb_connect_tick = HAL_GetTick();
      usb_action_due_tick = 0U;
      usb_msc_init_deadline_tick = HAL_GetTick() + USB_MSC_INIT_TIMEOUT_MS;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      usb_mount_pending = 0U;
      usb_mount_retry_count = 0U;
      usb_read_retry_count = 0U;
      usb_valid_update_found = 0U;
      usb_last_failure_reason_code = 0U;
      Appli_state = APPLICATION_START;
      break;

    case HOST_USER_CLASS_ACTIVE:
      USB_LOG("[USB] HOST_USER_CLASS_ACTIVE -> schedule mount %s\n", USBHPath);
      usb_seen_since_clear = 1U;
      usb_update_check_pending = 1U;
      usb_update_check_done = 0U;
      usb_action_due_tick = HAL_GetTick() + USB_MOUNT_SETTLE_DELAY_MS;
      usb_msc_init_deadline_tick = 0U;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      usb_mount_pending = 1U;
      usb_mount_retry_count = 0U;
      usb_read_retry_count = 0U;
      usb_valid_update_found = 0U;
      usb_last_failure_reason_code = 0U;
      Appli_state = APPLICATION_READY;
      break;

    case HOST_USER_DISCONNECTION:
      USB_LOG("[USB] HOST_USER_DISCONNECTION -> unmount %s\n", USBHPath);
      mount_res = USB_FATFS_Unmount();
      (void)mount_res;
      usb_update_check_pending = 0U;
      usb_update_check_done = 0U;
      usb_action_due_tick = 0U;
      usb_msc_init_deadline_tick = 0U;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      usb_mount_pending = 0U;
      usb_mount_retry_count = 0U;
      usb_read_retry_count = 0U;
      usb_valid_update_found = 0U;
      Appli_state = APPLICATION_DISCONNECT;
      break;

    case HOST_USER_UNRECOVERED_ERROR:
      USBH_AppPowerCycle(1U);
      break;

    default:
      break;
  }
  /* USER CODE END CALL_BACK_1 */
}

static uint8_t USBH_AppIsTransientFatFsError(FRESULT res)
{
  switch (res)
  {
    case FR_DISK_ERR:
    case FR_NOT_READY:
    case FR_NO_FILESYSTEM:
      return 1U;

    default:
      return 0U;
  }
}

static uint8_t USBH_AppIsLunReady(uint8_t lun)
{
  if ((hUsbHostHS.pActiveClass == NULL) || (hUsbHostHS.pActiveClass->pData == NULL))
  {
    return 0U;
  }

  return USBH_MSC_UnitIsReady(&hUsbHostHS, lun);
}

static uint8_t USBH_AppTimeReached(uint32_t due_tick)
{
  return ((int32_t)(HAL_GetTick() - due_tick) >= 0) ? 1U : 0U;
}

static void USBH_AppPowerCycle(uint32_t reason_code)
{
  FRESULT mount_res;

  if (usb_media_recovery_attempted != 0U)
  {
    usb_update_check_done = 1U;
    return;
  }

  usb_media_recovery_attempted = 1U;
  usb_last_failure_reason_code = reason_code;

  if (usb_power_cycle_count >= USB_POWER_CYCLE_MAX_COUNT)
  {
    USB_LOG("\n[USB] ========================================\n");
    USB_LOG("[USB] FINAL USB RECOVERY FAILURE\n");
    USB_LOG("[USB] power cycles exhausted: %u\n", (unsigned)usb_power_cycle_count);
    USB_LOG("[USB] last reason code: %lu\n", (unsigned long)reason_code);
    USB_LOG("[USB] SYSTEM RESET in %u ms\n", (unsigned)USB_FINAL_RESET_DELAY_MS);
    USB_LOG("[USB] ========================================\n\n");

    usb_update_check_pending = 0U;
    usb_update_check_done = 1U;
    usb_mount_pending = 0U;
    usb_mount_retry_count = 0U;
    usb_read_retry_count = 0U;
    usb_valid_update_found = 0U;
    usb_action_due_tick = 0U;
    usb_msc_init_deadline_tick = 0U;
    HAL_Delay(USB_FINAL_RESET_DELAY_MS);
    NVIC_SystemReset();
    return;
  }

  usb_power_cycle_count++;

  USB_LOG("\n[USB] ========================================\n");
  USB_LOG("[USB] POWER CYCLE USB VBUS\n");
  USB_LOG("[USB] cycle: %u/%u\n",
          (unsigned)usb_power_cycle_count,
          (unsigned)USB_POWER_CYCLE_MAX_COUNT);
  USB_LOG("[USB] reason code: %lu\n", (unsigned long)reason_code);
  USB_LOG("[USB] ========================================\n\n");

  mount_res = USB_FATFS_Unmount();
  (void)mount_res;

  usb_update_check_pending = 0U;
  usb_update_check_done = 0U;
  usb_mount_pending = 0U;
  usb_mount_retry_count = 0U;
  usb_read_retry_count = 0U;
  usb_valid_update_found = 0U;
  usb_action_due_tick = 0U;
  usb_msc_init_deadline_tick = 0U;

  hUsbHostHS.device.is_ReEnumerated = 1U;
  (void)USBH_Stop(&hUsbHostHS);
  HAL_Delay(USB_POWER_CYCLE_OFF_MS);
}

static uint8_t USBH_AppValidateUpdateImage(void)
{
  if (USBH_AppValidateOneImage(USB_UPDATE_INT_BIN_PATH,
                               USB_UPDATE_INT_CRC_PATH,
                               (APP_END - APP_BASE + 1U),
                               "internal") == 0U)
  {
    return 0U;
  }

  if (USBH_AppValidateOneImage(USB_UPDATE_OSPI_BIN_PATH,
                               USB_UPDATE_OSPI_CRC_PATH,
                               USB_OSPI_SIZE_BYTES,
                               "ospi") == 0U)
  {
    return 0U;
  }

  USB_LOG("[USB] valid split update detected\n");
  return 1U;
}

static uint8_t USBH_AppValidateOneImage(const char *bin_path,
                                        const char *crc_path,
                                        uint32_t max_size_bytes,
                                        const char *label)
{
  FRESULT res;
  FSIZE_t bin_size = 0U;
  FSIZE_t crc_size = 0U;
  uint32_t expected_crc;
  uint32_t computed_crc;
  char crc_text[32];
  char *end_ptr;

  res = USB_FATFS_GetFileSize(bin_path, &bin_size);
  if (res != FR_OK)
  {
    USB_LOG("[USB] %s image missing or invalid: %d\n", label, res);
    return 0U;
  }

  if (((uint32_t)bin_size == 0U) || ((uint32_t)bin_size > max_size_bytes))
  {
    USB_LOG("[USB] %s image size invalid: %lu bytes (max %lu)\n",
            label,
            (unsigned long)bin_size,
            (unsigned long)max_size_bytes);
    return 0U;
  }

  res = USB_FATFS_ReadTextFile(crc_path, crc_text, sizeof(crc_text));
  if (res != FR_OK)
  {
    USB_LOG("[USB] %s CRC file missing or invalid: %d\n", label, res);
    return 0U;
  }
  USB_LOG("[USB] %s CRC text read OK\n", label);

  expected_crc = (uint32_t)strtoul(crc_text, &end_ptr, 16);
  if (end_ptr == crc_text)
  {
    USB_LOG("[USB] invalid %s CRC text: %s\n", label, crc_text);
    return 0U;
  }
  USB_LOG("[USB] %s CRC text parsed OK\n", label);

  res = USB_FATFS_ComputeFileCrc32(bin_path, &computed_crc, &crc_size);
  if (res != FR_OK)
  {
    USB_LOG("[USB] %s CRC computation failed: %d\n", label, res);
    return 0U;
  }
  USB_LOG("[USB] %s CRC computation OK\n", label);

  if (crc_size != bin_size)
  {
    USB_LOG("[USB] %s size changed during CRC: stat=%lu crc=%lu\n",
            label,
            (unsigned long)bin_size,
            (unsigned long)crc_size);
    return 0U;
  }

  if (computed_crc != expected_crc)
  {
    USB_LOG("[USB] %s CRC mismatch: expected=0x%08lX computed=0x%08lX\n",
            label,
            (unsigned long)expected_crc,
            (unsigned long)computed_crc);
    return 0U;
  }

  USB_LOG("[USB] valid %s image detected: %s + %s\n",
          label,
          bin_path,
          crc_path);
  return 1U;
}
