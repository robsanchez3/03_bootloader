#include "usb_update.h"
#include "usb_msc_service.h"

static UsbUpdateFileInfo_t usb_update_files[USB_UPDATE_FILE_COUNT] =
{
    { USB_UPDATE_APP_INT_BIN, 0U },
    { USB_UPDATE_APP_OSPI_BIN, 0U },
    { USB_UPDATE_APP_INT_CRC, 0U },
    { USB_UPDATE_APP_OSPI_CRC, 0U }
};

void UsbUpdate_Init(void)
{
    uint32_t index;

    UsbMscService_Init();
    UsbFsService_Init();

    for (index = 0U; index < USB_UPDATE_FILE_COUNT; index++)
    {
        usb_update_files[index].present = 0U;
    }
}

void UsbUpdate_Process(void)
{
    UsbMscService_Process();
    UsbFsService_Process();
}

void UsbUpdate_Reset(void)
{
    uint32_t index;

    UsbFsService_Reset();
    UsbMscService_SetEnabled(0U);

    for (index = 0U; index < USB_UPDATE_FILE_COUNT; index++)
    {
        usb_update_files[index].present = 0U;
    }
}

uint8_t UsbUpdate_DeviceConnected(void)
{
    return UsbMscService_IsDeviceConnected();
}

uint8_t UsbUpdate_DeviceReady(void)
{
    return UsbMscService_IsReady();
}

uint8_t UsbUpdate_FilesPresent(void)
{
    uint32_t index;

    for (index = 0U; index < USB_UPDATE_FILE_COUNT; index++)
    {
        if (usb_update_files[index].present == 0U)
        {
            return 0U;
        }
    }

    return 1U;
}

UsbFsResult_t UsbUpdate_ProbeFiles(void)
{
    uint32_t index;
    UsbFsResult_t result;

    for (index = 0U; index < USB_UPDATE_FILE_COUNT; index++)
    {
        result = UsbFsService_FileExists(usb_update_files[index].path);
        usb_update_files[index].present = (result == USB_FS_RESULT_OK) ? 1U : 0U;

        if (result != USB_FS_RESULT_OK)
        {
            return result;
        }
    }

    return USB_FS_RESULT_OK;
}

UsbFsResult_t UsbUpdate_ReadFile(UsbUpdateFileId_t file_id,
                                 void *buffer,
                                 uint32_t offset,
                                 uint32_t bytes_to_read,
                                 uint32_t *bytes_read)
{
    const UsbUpdateFileInfo_t *file_info;

    file_info = UsbUpdate_GetFileInfo(file_id);
    if (file_info == NULL)
    {
        return USB_FS_RESULT_INVALID_ARG;
    }

    return UsbFsService_ReadFile(file_info->path,
                                 buffer,
                                 offset,
                                 bytes_to_read,
                                 bytes_read);
}

const UsbUpdateFileInfo_t *UsbUpdate_GetFileInfo(UsbUpdateFileId_t file_id)
{
    if ((uint32_t)file_id >= USB_UPDATE_FILE_COUNT)
    {
        return (const UsbUpdateFileInfo_t *)0;
    }

    return &usb_update_files[file_id];
}
