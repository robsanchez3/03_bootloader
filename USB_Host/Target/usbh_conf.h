/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : USB_Host/Target/usbh_conf.h
  * @brief          : USB Host low level configuration - STM32U5 adaptation
  *
  * Adapted from OztDUI_UsbFlash (STM32F7) reference project.
  * Key changes vs reference:
  *   - stm32f7xx.h -> stm32u5xx.h
  *   - USB_OTG_FS  -> USB_OTG_HS (embedded PHY, High Speed)
  *   - HOST_FS=0, HOST_HS=0  (only HS peripheral exists on STM32U5)
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USBH_CONF__H__
#define __USBH_CONF__H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/*----------   -----------*/
#define USBH_MAX_NUM_ENDPOINTS      2U

/*----------   -----------*/
#define USBH_MAX_NUM_INTERFACES     2U

/*----------   -----------*/
#define USBH_MAX_NUM_CONFIGURATION  1U

/*----------   -----------*/
#define USBH_KEEP_CFG_DESCRIPTOR    1U

/*----------   -----------*/
#define USBH_MAX_NUM_SUPPORTED_CLASS  1U

/*----------   -----------*/
#define USBH_MAX_SIZE_CONFIGURATION   256U

/*----------   -----------*/
#define USBH_MAX_DATA_BUFFER          512U

/*----------   -----------*/
#define USBH_DEBUG_LEVEL              1U

/* Retry IN transfers that answer with NAK instead of stalling forever */
#define USBH_IN_NAK_PROCESS           1U

/*----------   -----------*/
#define USBH_USE_OS                   0U

/* HS/FS identification - STM32U5 only has USB_OTG_HS */
#define HOST_HS   0
#define HOST_FS   1

#if (USBH_USE_OS == 1U)
  #include "cmsis_os.h"
  #define USBH_PROCESS_PRIO          osPriorityNormal
  #define USBH_PROCESS_STACK_SIZE    ((uint16_t)0)
#endif

/* Memory management - use FreeRTOS heap */
#define USBH_malloc malloc
#define USBH_free   free
#define USBH_memset memset
#define USBH_memcpy memcpy

/* DEBUG macros */
#if (USBH_DEBUG_LEVEL > 0U)
#define USBH_UsrLog(...)  do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_UsrLog(...) do {} while (0)
#endif

#if (USBH_DEBUG_LEVEL > 1U)
#define USBH_ErrLog(...)  do { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_ErrLog(...) do {} while (0)
#endif

#if (USBH_DEBUG_LEVEL > 2U)
#define USBH_DbgLog(...)  do { printf("DEBUG: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_DbgLog(...) do {} while (0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* __USBH_CONF__H__ */
