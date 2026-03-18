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
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
USBH_HandleTypeDef  hUsbHostHS;
ApplicationTypeDef  Appli_state = APPLICATION_IDLE;

/* USER CODE BEGIN PV */
static uint8_t usb_test_read_pending;
static uint8_t usb_test_read_done;
static uint32_t usb_connect_tick;
static uint8_t usb_reenum_attempted;
static uint8_t usb_media_recovery_attempted;

#define USB_ENUM_TIMEOUT_MS 3000U

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/* USER CODE BEGIN PFP */
static uint8_t USBH_AppIsTransientFatFsError(FRESULT res);
static uint8_t USBH_AppIsLunReady(uint8_t lun);

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
      (usb_reenum_attempted == 0U) &&
      ((HAL_GetTick() - usb_connect_tick) >= USB_ENUM_TIMEOUT_MS))
  {
    USB_LOG("[USB] enum timeout -> re-enumerate\n");
    usb_reenum_attempted = 1U;
    (void)USBH_ReEnumerate(&hUsbHostHS);
    usb_connect_tick = HAL_GetTick();
  }
}

void USBH_AppTask(void)
{
  FILINFO fno;
  FRESULT fat_res;
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

  fat_res = f_stat("0:/UPDATE/test.txt", &fno);
  USB_LOG("[USB] f_stat result = %d\n", fat_res);
  if (fat_res == FR_OK)
  {
    USB_LOG("[USB] reading 0:/UPDATE/test.txt\n");
    read_file_res = USB_FATFS_ReadTestFile("0:/UPDATE/test.txt");
    USB_LOG("[USB] file read result = %d\n", read_file_res);

    if (read_file_res == FR_OK)
    {
      usb_test_read_done = 1U;
      return;
    }

    if ((USBH_AppIsTransientFatFsError(read_file_res) != 0U) &&
        (usb_media_recovery_attempted == 0U))
    {
      USB_LOG("[USB] media error -> re-enumerate\n");
      usb_media_recovery_attempted = 1U;
      (void)USBH_ReEnumerate(&hUsbHostHS);
      return;
    }

    if (USBH_AppIsTransientFatFsError(read_file_res) == 0U)
    {
      usb_test_read_done = 1U;
      return;
    }
  }
  else
  {
    if ((USBH_AppIsTransientFatFsError(fat_res) != 0U) &&
        (usb_media_recovery_attempted == 0U))
    {
      USB_LOG("[USB] media error -> re-enumerate\n");
      usb_media_recovery_attempted = 1U;
      (void)USBH_ReEnumerate(&hUsbHostHS);
      return;
    }

    if (USBH_AppIsTransientFatFsError(fat_res) == 0U)
    {
      usb_test_read_done = 1U;
      return;
    }
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
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      Appli_state = APPLICATION_START;
      break;

    case HOST_USER_CLASS_ACTIVE:
      USB_LOG("[USB] HOST_USER_CLASS_ACTIVE -> mount %s\n", USBHPath);
      mount_res = f_mount(&USBHFatFS, (TCHAR const*)USBHPath, 1);
      USB_LOG("[USB] f_mount result = %d\n", mount_res);
      if (mount_res == FR_OK)
      {
        usb_test_read_pending = 1U;
        usb_test_read_done = 0U;
      }
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      Appli_state = APPLICATION_READY;
      break;

    case HOST_USER_DISCONNECTION:
      USB_LOG("[USB] HOST_USER_DISCONNECTION -> unmount %s\n", USBHPath);
      mount_res = f_mount(NULL, (TCHAR const*)USBHPath, 0);
      USB_LOG("[USB] f_unmount result = %d\n", mount_res);
      usb_test_read_pending = 0U;
      usb_test_read_done = 0U;
      usb_reenum_attempted = 0U;
      usb_media_recovery_attempted = 0U;
      Appli_state = APPLICATION_DISCONNECT;
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
