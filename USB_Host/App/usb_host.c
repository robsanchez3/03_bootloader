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
#include <stdio.h>

/* USER CODE BEGIN Includes */
#define USB_HOST_DEBUG
#ifdef USB_HOST_DEBUG
#define USB_LOG(...)  printf(__VA_ARGS__)
#else
#define USB_LOG(...)  do {} while(0)
#endif

#define USB_TEST_FILE_PATH           "0:/UPDATE/test.txt"
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
static uint8_t usb_test_read_pending;
static uint8_t usb_test_read_done;
static uint32_t usb_connect_tick;
static uint32_t usb_action_due_tick;
static uint32_t usb_msc_init_deadline_tick;
static uint8_t usb_reenum_attempted;
static uint8_t usb_media_recovery_attempted;
static uint8_t usb_mount_pending;
static uint8_t usb_mount_retry_count;
static uint8_t usb_read_retry_count;
static uint8_t usb_power_cycle_count;
static const char *usb_last_failure_reason;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/* USER CODE BEGIN PFP */
static uint8_t USBH_AppIsTransientFatFsError(FRESULT res);
static uint8_t USBH_AppIsLunReady(uint8_t lun);
static uint8_t USBH_AppTimeReached(uint32_t due_tick);
static void USBH_AppPowerCycle(const char *reason);

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
    USBH_AppPowerCycle("MSC init watchdog timeout");
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

  if ((usb_test_read_pending == 0U) || (usb_test_read_done != 0U))
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

    USBH_AppPowerCycle("mount failed after retries");
    return;
  }

  USB_LOG("[USB] reading %s\n", USB_TEST_FILE_PATH);
  read_file_res = USB_FATFS_ReadTestFile(USB_TEST_FILE_PATH);
  USB_LOG("[USB] file read result = %d\n", read_file_res);

  if (read_file_res == FR_OK)
  {
    usb_read_retry_count = 0U;
    usb_power_cycle_count = 0U;
    usb_last_failure_reason = NULL;
    usb_test_read_done = 1U;
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
    USBH_AppPowerCycle("file read failed after retries");
    return;
  }

  usb_test_read_done = 1U;
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
      usb_test_read_pending = 0U;
      usb_test_read_done = 0U;
      usb_connect_tick = HAL_GetTick();
      usb_action_due_tick = 0U;
      usb_msc_init_deadline_tick = HAL_GetTick() + USB_MSC_INIT_TIMEOUT_MS;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      usb_mount_pending = 0U;
      usb_mount_retry_count = 0U;
      usb_read_retry_count = 0U;
      usb_last_failure_reason = NULL;
      Appli_state = APPLICATION_START;
      break;

    case HOST_USER_CLASS_ACTIVE:
      USB_LOG("[USB] HOST_USER_CLASS_ACTIVE -> schedule mount %s\n", USBHPath);
      usb_test_read_pending = 1U;
      usb_test_read_done = 0U;
      usb_action_due_tick = HAL_GetTick() + USB_MOUNT_SETTLE_DELAY_MS;
      usb_msc_init_deadline_tick = 0U;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      usb_mount_pending = 1U;
      usb_mount_retry_count = 0U;
      usb_read_retry_count = 0U;
      usb_last_failure_reason = NULL;
      Appli_state = APPLICATION_READY;
      break;

    case HOST_USER_DISCONNECTION:
      USB_LOG("[USB] HOST_USER_DISCONNECTION -> unmount %s\n", USBHPath);
      mount_res = USB_FATFS_Unmount();
      (void)mount_res;
      usb_test_read_pending = 0U;
      usb_test_read_done = 0U;
      usb_action_due_tick = 0U;
      usb_msc_init_deadline_tick = 0U;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      usb_mount_pending = 0U;
      usb_mount_retry_count = 0U;
      usb_read_retry_count = 0U;
      Appli_state = APPLICATION_DISCONNECT;
      break;

    case HOST_USER_UNRECOVERED_ERROR:
      USBH_AppPowerCycle("HOST_USER_UNRECOVERED_ERROR");
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

static void USBH_AppPowerCycle(const char *reason)
{
  FRESULT mount_res;

  if (usb_media_recovery_attempted != 0U)
  {
    usb_test_read_done = 1U;
    return;
  }

  usb_media_recovery_attempted = 1U;
  usb_last_failure_reason = reason;

  if (usb_power_cycle_count >= USB_POWER_CYCLE_MAX_COUNT)
  {
    USB_LOG("\n[USB] ========================================\n");
    USB_LOG("[USB] FINAL USB RECOVERY FAILURE\n");
    USB_LOG("[USB] power cycles exhausted: %u\n", (unsigned)usb_power_cycle_count);
    USB_LOG("[USB] last reason: %s\n", reason);
    USB_LOG("[USB] SYSTEM RESET in %u ms\n", (unsigned)USB_FINAL_RESET_DELAY_MS);
    USB_LOG("[USB] ========================================\n\n");

    usb_test_read_pending = 0U;
    usb_test_read_done = 1U;
    usb_mount_pending = 0U;
    usb_mount_retry_count = 0U;
    usb_read_retry_count = 0U;
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
  USB_LOG("[USB] reason: %s\n", reason);
  USB_LOG("[USB] ========================================\n\n");

  mount_res = USB_FATFS_Unmount();
  (void)mount_res;

  usb_test_read_pending = 0U;
  usb_test_read_done = 0U;
  usb_mount_pending = 0U;
  usb_mount_retry_count = 0U;
  usb_read_retry_count = 0U;
  usb_action_due_tick = 0U;
  usb_msc_init_deadline_tick = 0U;

  hUsbHostHS.device.is_ReEnumerated = 1U;
  (void)USBH_Stop(&hUsbHostHS);
  HAL_Delay(USB_POWER_CYCLE_OFF_MS);
}
