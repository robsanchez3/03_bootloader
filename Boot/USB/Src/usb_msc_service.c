#include "usb_msc_service.h"
#include "usbh_conf.h"
#include "usbh_msc.h"
#include <stdio.h>

#define USB_MSC_ENUM_START_TIMEOUT_MS   3000U
#define USB_MSC_READY_TIMEOUT_MS        8000U
#define USB_MSC_MAX_RECOVERY_RESTARTS   3U
#define USB_MSC_VBUS_DELAY_NORMAL_MS    750U
#define USB_MSC_VBUS_DELAY_RECOVERY_MS  1500U

USBH_HandleTypeDef hUsbHostHS;

static uint8_t usb_msc_enabled;
static uint8_t usb_msc_host_initialized;
static UsbMscState_t usb_msc_state;
static uint32_t usb_msc_last_error;
static uint32_t usb_msc_attempt_count;
static uint32_t usb_msc_attempt_start_tick;
static uint32_t usb_msc_enum_start_tick;
static uint8_t usb_msc_seen_enumerating;
static uint8_t usb_msc_timeout_reported;
static uint8_t usb_msc_recovery_restarts;

static void UsbMscService_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
static void UsbMscService_SetState(UsbMscState_t state);
static void UsbMscService_RestartHost(void);
static void UsbMscService_TriggerRecovery(const char *reason);

void UsbMscService_Init(void)
{
    usb_msc_enabled = 0U;
    usb_msc_host_initialized = 0U;
    usb_msc_state = USB_MSC_STATE_OFF;
    usb_msc_last_error = 0U;
    usb_msc_attempt_count = 0U;
    usb_msc_attempt_start_tick = 0U;
    usb_msc_enum_start_tick = 0U;
    usb_msc_seen_enumerating = 0U;
    usb_msc_timeout_reported = 0U;
    usb_msc_recovery_restarts = 0U;
    USBH_LL_SetVbusDelay(USB_MSC_VBUS_DELAY_NORMAL_MS);
}

void UsbMscService_Process(void)
{
    if (usb_msc_enabled == 0U)
    {
        usb_msc_state = USB_MSC_STATE_OFF;
        return;
    }

    if (usb_msc_host_initialized == 0U)
    {
        USBH_LL_SetVbusDelay((usb_msc_recovery_restarts != 0U) ?
                             USB_MSC_VBUS_DELAY_RECOVERY_MS :
                             USB_MSC_VBUS_DELAY_NORMAL_MS);

        if (USBH_Init(&hUsbHostHS, UsbMscService_UserProcess, HOST_HS) != USBH_OK)
        {
            usb_msc_last_error = 1U;
            UsbMscService_SetState(USB_MSC_STATE_ERROR);
            return;
        }

        if (USBH_RegisterClass(&hUsbHostHS, USBH_MSC_CLASS) != USBH_OK)
        {
            usb_msc_last_error = 2U;
            UsbMscService_SetState(USB_MSC_STATE_ERROR);
            return;
        }

        if (USBH_Start(&hUsbHostHS) != USBH_OK)
        {
            usb_msc_last_error = 3U;
            UsbMscService_SetState(USB_MSC_STATE_ERROR);
            return;
        }

        usb_msc_host_initialized = 1U;
        UsbMscService_SetState(USB_MSC_STATE_IDLE);
    }

    (void)USBH_Process(&hUsbHostHS);

    if ((hUsbHostHS.pActiveClass == USBH_MSC_CLASS) && (USBH_MSC_IsReady(&hUsbHostHS) != 0U))
    {
        if (usb_msc_state != USB_MSC_STATE_READY)
        {
            printf("[USB] attempt %lu READY in %lu ms\n",
                   (unsigned long)usb_msc_attempt_count,
                   (unsigned long)(HAL_GetTick() - usb_msc_attempt_start_tick));
        }
        UsbMscService_SetState(USB_MSC_STATE_READY);
    }
    else if (usb_msc_state == USB_MSC_STATE_READY)
    {
        UsbMscService_SetState(USB_MSC_STATE_ENUMERATING);
    }

    if ((usb_msc_state == USB_MSC_STATE_DEVICE_CONNECTED) && (usb_msc_timeout_reported == 0U))
    {
        if ((HAL_GetTick() - usb_msc_attempt_start_tick) >= USB_MSC_ENUM_START_TIMEOUT_MS)
        {
            usb_msc_timeout_reported = 1U;
            usb_msc_last_error = 10U;
            printf("[USB] attempt %lu timeout before ENUMERATING\n",
                   (unsigned long)usb_msc_attempt_count);
            UsbMscService_TriggerRecovery("timeout before ENUMERATING");
            return;
        }
    }

    if ((usb_msc_state == USB_MSC_STATE_ENUMERATING) && (usb_msc_timeout_reported == 0U))
    {
        if ((HAL_GetTick() - usb_msc_enum_start_tick) >= USB_MSC_READY_TIMEOUT_MS)
        {
            usb_msc_timeout_reported = 1U;
            usb_msc_last_error = 11U;
            printf("[USB] attempt %lu timeout before READY\n",
                   (unsigned long)usb_msc_attempt_count);
        }
    }
}

void UsbMscService_SetEnabled(uint8_t enabled)
{
    usb_msc_enabled = (enabled != 0U) ? 1U : 0U;

    if (usb_msc_enabled == 0U)
    {
        if (usb_msc_host_initialized != 0U)
        {
            (void)USBH_Stop(&hUsbHostHS);
            (void)USBH_DeInit(&hUsbHostHS);
            usb_msc_host_initialized = 0U;
        }

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

const char *UsbMscService_GetStateName(void)
{
    switch (usb_msc_state)
    {
        case USB_MSC_STATE_OFF:
            return "OFF";
        case USB_MSC_STATE_IDLE:
            return "IDLE";
        case USB_MSC_STATE_DEVICE_CONNECTED:
            return "DEVICE_CONNECTED";
        case USB_MSC_STATE_ENUMERATING:
            return "ENUMERATING";
        case USB_MSC_STATE_READY:
            return "READY";
        case USB_MSC_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

uint32_t UsbMscService_GetLastError(void)
{
    return usb_msc_last_error;
}

static void UsbMscService_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
    (void)phost;

    switch (id)
    {
        case HOST_USER_CONNECTION:
            usb_msc_attempt_count++;
            usb_msc_attempt_start_tick = HAL_GetTick();
            usb_msc_enum_start_tick = 0U;
            usb_msc_seen_enumerating = 0U;
            usb_msc_timeout_reported = 0U;
            printf("[USB] attempt %lu connected\n", (unsigned long)usb_msc_attempt_count);
            UsbMscService_SetState(USB_MSC_STATE_DEVICE_CONNECTED);
            break;

        case HOST_USER_CLASS_ACTIVE:
            if (usb_msc_seen_enumerating == 0U)
            {
                usb_msc_seen_enumerating = 1U;
                usb_msc_enum_start_tick = HAL_GetTick();
                printf("[USB] attempt %lu ENUMERATING after %lu ms\n",
                       (unsigned long)usb_msc_attempt_count,
                       (unsigned long)(usb_msc_enum_start_tick - usb_msc_attempt_start_tick));
            }
            UsbMscService_SetState(USB_MSC_STATE_ENUMERATING);
            break;

        case HOST_USER_DISCONNECTION:
            if (usb_msc_state == USB_MSC_STATE_DEVICE_CONNECTED)
            {
                printf("[USB] attempt %lu disconnected before ENUMERATING\n",
                       (unsigned long)usb_msc_attempt_count);
            }
            else if (usb_msc_state == USB_MSC_STATE_ENUMERATING)
            {
                printf("[USB] attempt %lu disconnected during ENUMERATING\n",
                       (unsigned long)usb_msc_attempt_count);
            }
            else if (usb_msc_state == USB_MSC_STATE_READY)
            {
                printf("[USB] attempt %lu disconnected after READY\n",
                       (unsigned long)usb_msc_attempt_count);
            }
            uint8_t was_ready = (usb_msc_state == USB_MSC_STATE_READY);
            uint8_t was_enum = (usb_msc_state == USB_MSC_STATE_ENUMERATING);
            uint8_t was_connected = (usb_msc_state == USB_MSC_STATE_DEVICE_CONNECTED);

            UsbMscService_SetState(USB_MSC_STATE_IDLE);

            if ((was_connected || was_enum) && !was_ready)
            {
                UsbMscService_TriggerRecovery("disconnected before ENUMERATING");
            }
            break;

        case HOST_USER_UNRECOVERED_ERROR:
            usb_msc_last_error = 4U;
            /* During bring-up, some devices fail GetMaxLUN but still continue
               correctly as single-LUN MSC devices. Keep the service in the
               current flow and let Process() promote to READY if the stack
               recovers and progresses. */
            if (usb_msc_state < USB_MSC_STATE_DEVICE_CONNECTED)
            {
                UsbMscService_SetState(USB_MSC_STATE_ERROR);
            }
            break;

        default:
            break;
    }
}

static void UsbMscService_SetState(UsbMscState_t state)
{
    usb_msc_state = state;
    if (state == USB_MSC_STATE_READY)
    {
        usb_msc_recovery_restarts = 0U;
        USBH_LL_SetVbusDelay(USB_MSC_VBUS_DELAY_NORMAL_MS);
    }
}

static void UsbMscService_RestartHost(void)
{
    if (usb_msc_host_initialized != 0U)
    {
        (void)USBH_Stop(&hUsbHostHS);
        (void)USBH_DeInit(&hUsbHostHS);
        usb_msc_host_initialized = 0U;
    }

    usb_msc_state = USB_MSC_STATE_IDLE;
    usb_msc_attempt_start_tick = HAL_GetTick();
    usb_msc_enum_start_tick = 0U;
    usb_msc_seen_enumerating = 0U;
    usb_msc_timeout_reported = 0U;
}

void UsbMscService_ForceRestart(void)
{
    printf("[USB] ForceRestart: hard reset + VBUS recovery\n");
    USBH_LL_SetVbusDelay(USB_MSC_VBUS_DELAY_RECOVERY_MS);
    USBH_LL_HardResetHostController();
    UsbMscService_RestartHost();
}

static void UsbMscService_TriggerRecovery(const char *reason)
{
    if (usb_msc_recovery_restarts >= USB_MSC_MAX_RECOVERY_RESTARTS)
    {
        return;
    }

    usb_msc_recovery_restarts++;
    printf("[USB] attempt %lu %s -> restarting host (%u/%u)\n",
           (unsigned long)usb_msc_attempt_count,
           reason,
           (unsigned int)usb_msc_recovery_restarts,
           (unsigned int)USB_MSC_MAX_RECOVERY_RESTARTS);
    printf("[USB] hard reset USB + recovery VBUS delay %u ms\n",
           (unsigned int)USB_MSC_VBUS_DELAY_RECOVERY_MS);
    USBH_LL_SetVbusDelay(USB_MSC_VBUS_DELAY_RECOVERY_MS);
    USBH_LL_HardResetHostController();
    UsbMscService_RestartHost();
}
