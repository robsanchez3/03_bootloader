#ifndef __USBH_CONF__H__
#define __USBH_CONF__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USBH_MAX_NUM_ENDPOINTS         2U
#define USBH_MAX_NUM_INTERFACES        2U
#define USBH_MAX_NUM_CONFIGURATION     1U
#define USBH_KEEP_CFG_DESCRIPTOR       1U
#define USBH_MAX_NUM_SUPPORTED_CLASS   1U
#define USBH_MAX_SIZE_CONFIGURATION    256U
#define USBH_MAX_DATA_BUFFER           512U
#define USBH_DEBUG_LEVEL               0U
#define USBH_IN_NAK_PROCESS            1U
#define USBH_NAK_SOF_COUNT             20U
#define USBH_USE_OS                    0U
#define BOOT_USB_ST_TRACE_VERBOSE      0U

#define HOST_HS                        1U

#define USBH_malloc                    malloc
#define USBH_free                      free
#define USBH_memset                    memset
#define USBH_memcpy                    memcpy

#if (USBH_DEBUG_LEVEL > 0U)
#define USBH_UsrLog(...) do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_UsrLog(...) do { } while (0)
#endif

#if (USBH_DEBUG_LEVEL > 1U)
#define USBH_ErrLog(...) do { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_ErrLog(...) do { } while (0)
#endif

#if (USBH_DEBUG_LEVEL > 2U)
#define USBH_DbgLog(...) do { printf("DEBUG: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_DbgLog(...) do { } while (0)
#endif

#if (BOOT_USB_ST_TRACE_VERBOSE != 0U)
#define BOOT_USB_ST_TRACE(...) do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define BOOT_USB_ST_TRACE(...) do { } while (0)
#endif

void USBH_LL_SetVbusDelay(uint32_t delay_ms);
void USBH_LL_HardResetHostController(void);

#ifdef __cplusplus
}
#endif

#endif /* __USBH_CONF__H__ */
