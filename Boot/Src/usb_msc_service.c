#include "usb_msc_service.h"

static uint8_t usb_msc_enabled;
static UsbMscState_t usb_msc_state;
static uint32_t usb_msc_last_error;

void UsbMscService_Init(void)
{
    usb_msc_enabled = 0U;
    usb_msc_state = USB_MSC_STATE_IDLE;
    usb_msc_last_error = 0U;
}

void UsbMscService_Process(void)
{
    if (usb_msc_enabled == 0U)
    {
        usb_msc_state = USB_MSC_STATE_OFF;
        return;
    }

    if (usb_msc_state == USB_MSC_STATE_OFF)
    {
        usb_msc_state = USB_MSC_STATE_IDLE;
    }

    /* Phase 2 stub: real USB host + MSC state machine will be added here. */
}

void UsbMscService_SetEnabled(uint8_t enabled)
{
    usb_msc_enabled = (enabled != 0U) ? 1U : 0U;

    if (usb_msc_enabled == 0U)
    {
        usb_msc_state = USB_MSC_STATE_OFF;
    }
    else if (usb_msc_state == USB_MSC_STATE_OFF)
    {
        usb_msc_state = USB_MSC_STATE_IDLE;
    }
}

uint8_t UsbMscService_IsEnabled(void)
{
    return usb_msc_enabled;
}

uint8_t UsbMscService_IsDeviceConnected(void)
{
    return (usb_msc_state >= USB_MSC_STATE_DEVICE_CONNECTED) ? 1U : 0U;
}

uint8_t UsbMscService_IsReady(void)
{
    return (usb_msc_state == USB_MSC_STATE_READY) ? 1U : 0U;
}

UsbMscState_t UsbMscService_GetState(void)
{
    return usb_msc_state;
}

uint32_t UsbMscService_GetLastError(void)
{
    return usb_msc_last_error;
}
