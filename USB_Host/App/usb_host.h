/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : USB_Host/App/usb_host.h
  * @brief          : USB Host application header - STM32U5 adaptation
  *
  * Adapted from OztDUI_UsbFlash (STM32F7) reference project.
  * Key changes: stm32f7xx -> stm32u5xx, HOST_FS -> HOST_HS, handle renamed
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"
#include "usbh_core.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** Status of the USB Host application state machine */
typedef enum {
  APPLICATION_IDLE       = 0,
  APPLICATION_START,
  APPLICATION_READY,
  APPLICATION_DISCONNECT
} ApplicationTypeDef;

/* Exported variables -------------------------------------------------------*/
extern USBH_HandleTypeDef   hUsbHostHS;
extern ApplicationTypeDef   Appli_state;

/* Exported functions -------------------------------------------------------*/
void    MX_USB_HOST_Init(void);
void    MX_USB_HOST_Process(void);
void    USBH_AppTask(void);
uint8_t USBH_IsFlashReady(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST__H__ */
