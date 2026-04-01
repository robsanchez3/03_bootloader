#ifndef USB_MSC_SERVICE_H
#define USB_MSC_SERVICE_H

#include <stdint.h>
#include "usbh_core.h"

typedef enum
{
    USB_MSC_STATE_OFF = 0,
    USB_MSC_STATE_IDLE,
    USB_MSC_STATE_DEVICE_CONNECTED,
    USB_MSC_STATE_ENUMERATING,
    USB_MSC_STATE_READY,
    USB_MSC_STATE_ERROR
} UsbMscState_t;

/* Poll-driven USB MSC host service used by the bootloader.
 * Call Init(), then SetEnabled(1), then poll Process() until the state reaches
 * READY or a higher-level timeout/policy decides otherwise. */
void UsbMscService_Init(void);
void UsbMscService_Process(void);
void UsbMscService_SetEnabled(uint8_t enabled);
uint8_t UsbMscService_IsEnabled(void);
uint8_t UsbMscService_IsDeviceConnected(void);
uint8_t UsbMscService_IsReady(void);
UsbMscState_t UsbMscService_GetState(void);
const char *UsbMscService_GetStateName(void);
uint32_t UsbMscService_GetLastError(void);
void UsbMscService_ForceRestart(void);

extern USBH_HandleTypeDef hUsbHostHS;

#endif /* USB_MSC_SERVICE_H */
