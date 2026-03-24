#include "usb_fs_service.h"
#include "usb_msc_service.h"

static uint8_t usb_fs_mounted;
static uint32_t usb_fs_last_error;

void UsbFsService_Init(void)
{
    usb_fs_mounted = 0U;
    usb_fs_last_error = 0U;
}

void UsbFsService_Process(void)
{
    if (UsbMscService_IsReady() == 0U)
    {
        usb_fs_mounted = 0U;
    }
}
asfas
void UsbFsService_Reset(void)
{
    usb_fs_mounted = 0U;
    usb_fs_last_error = 0U;
}

uint8_t UsbFsService_IsMounted(void)
{
    return usb_fs_mounted;
}

UsbFsResult_t UsbFsService_Mount(void)
{
    if (UsbMscService_IsReady() == 0U)
    {
        return USB_FS_RESULT_NOT_READY;
    }

    /* Phase 2 stub: real FatFs mount will be added here. */
    usb_fs_mounted = 0U;
    return USB_FS_RESULT_NOT_IMPLEMENTED;
}

UsbFsResult_t UsbFsService_Unmount(void)
{
    usb_fs_mounted = 0U;
    return USB_FS_RESULT_OK;
}

UsbFsResult_t UsbFsService_FileExists(const char *path)
{
    if (path == NULL)
    {
        return USB_FS_RESULT_INVALID_ARG;
    }

    if (usb_fs_mounted == 0U)
    {
        return USB_FS_RESULT_NOT_MOUNTED;
    }

    return USB_FS_RESULT_NOT_IMPLEMENTED;
}

UsbFsResult_t UsbFsService_ReadFile(const char *path,
                                    void *buffer,
                                    uint32_t offset,
                                    uint32_t bytes_to_read,
                                    uint32_t *bytes_read)
{
    (void)offset;

    if ((path == NULL) || (buffer == NULL) || (bytes_read == NULL) || (bytes_to_read == 0U))
    {
        return USB_FS_RESULT_INVALID_ARG;
    }

    *bytes_read = 0U;

    if (usb_fs_mounted == 0U)
    {
        return USB_FS_RESULT_NOT_MOUNTED;
    }

    return USB_FS_RESULT_NOT_IMPLEMENTED;
}

uint32_t UsbFsService_GetLastError(void)
{
    return usb_fs_last_error;
}
