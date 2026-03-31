#include "usb_fs_service.h"
#include "usb_msc_service.h"
#include "usbh_diskio.h"
#include "ff.h"
#include "ff_gen_drv.h"

static FATFS   usb_fs_object;
static char    usb_fs_path[4];
static uint8_t usb_fs_mounted;
static uint8_t usb_fs_driver_linked;
static uint32_t usb_fs_last_error;

void UsbFsService_Init(void)
{
    usb_fs_mounted       = 0U;
    usb_fs_driver_linked = 0U;
    usb_fs_last_error    = 0U;
}

void UsbFsService_Process(void)
{
    if (UsbMscService_IsReady() == 0U)
    {
        usb_fs_mounted = 0U;
    }
}

void UsbFsService_Reset(void)
{
    usb_fs_mounted       = 0U;
    usb_fs_driver_linked = 0U;
    usb_fs_last_error    = 0U;
}

uint8_t UsbFsService_IsMounted(void)
{
    return usb_fs_mounted;
}

UsbFsResult_t UsbFsService_Mount(void)
{
    FRESULT fr;

    if (UsbMscService_IsReady() == 0U)
    {
        return USB_FS_RESULT_NOT_READY;
    }

    if (usb_fs_driver_linked == 0U)
    {
        if (FATFS_LinkDriverEx(&USBH_Driver, usb_fs_path, 0U) != 0U)
        {
            usb_fs_last_error = (uint32_t)FR_DISK_ERR;
            return USB_FS_RESULT_IO_ERROR;
        }
        usb_fs_driver_linked = 1U;
    }

    fr = f_mount(&usb_fs_object, usb_fs_path, 1U);
    if (fr != FR_OK)
    {
        usb_fs_last_error = (uint32_t)fr;
        return USB_FS_RESULT_IO_ERROR;
    }

    usb_fs_mounted = 1U;
    return USB_FS_RESULT_OK;
}

UsbFsResult_t UsbFsService_Unmount(void)
{
    if (usb_fs_mounted != 0U)
    {
        (void)f_mount(NULL, usb_fs_path, 0U);
        usb_fs_mounted = 0U;
    }

    if (usb_fs_driver_linked != 0U)
    {
        (void)FATFS_UnLinkDriverEx(usb_fs_path, 0U);
        usb_fs_driver_linked = 0U;
    }

    return USB_FS_RESULT_OK;
}

UsbFsResult_t UsbFsService_FileExists(const char *path)
{
    FILINFO finfo;
    FRESULT fr;

    if (path == NULL)
    {
        return USB_FS_RESULT_INVALID_ARG;
    }

    if (usb_fs_mounted == 0U)
    {
        return USB_FS_RESULT_NOT_MOUNTED;
    }

    fr = f_stat(path, &finfo);

    if (fr == FR_OK)
    {
        return USB_FS_RESULT_OK;
    }

    if ((fr == FR_NO_FILE) || (fr == FR_NO_PATH))
    {
        usb_fs_last_error = (uint32_t)fr;
        return USB_FS_RESULT_NO_FILE;
    }

    usb_fs_last_error = (uint32_t)fr;
    return USB_FS_RESULT_IO_ERROR;
}

UsbFsResult_t UsbFsService_ReadFile(const char *path,
                                    void *buffer,
                                    uint32_t offset,
                                    uint32_t bytes_to_read,
                                    uint32_t *bytes_read)
{
    FIL    file;
    FRESULT fr;
    UINT   br;

    if ((path == NULL) || (buffer == NULL) || (bytes_read == NULL) || (bytes_to_read == 0U))
    {
        return USB_FS_RESULT_INVALID_ARG;
    }

    *bytes_read = 0U;

    if (usb_fs_mounted == 0U)
    {
        return USB_FS_RESULT_NOT_MOUNTED;
    }

    fr = f_open(&file, path, FA_READ);
    if (fr != FR_OK)
    {
        usb_fs_last_error = (uint32_t)fr;
        return ((fr == FR_NO_FILE) || (fr == FR_NO_PATH)) ?
               USB_FS_RESULT_NO_FILE : USB_FS_RESULT_IO_ERROR;
    }

    if (offset > 0U)
    {
        fr = f_lseek(&file, (FSIZE_t)offset);
        if (fr != FR_OK)
        {
            usb_fs_last_error = (uint32_t)fr;
            (void)f_close(&file);
            return USB_FS_RESULT_IO_ERROR;
        }
    }

    fr = f_read(&file, buffer, (UINT)bytes_to_read, &br);
    (void)f_close(&file);

    if (fr != FR_OK)
    {
        usb_fs_last_error = (uint32_t)fr;
        return USB_FS_RESULT_IO_ERROR;
    }

    *bytes_read = (uint32_t)br;
    return USB_FS_RESULT_OK;
}

UsbFsResult_t UsbFsService_GetFileSize(const char *path, uint32_t *file_size)
{
    FILINFO finfo;
    FRESULT fr;

    if ((path == NULL) || (file_size == NULL))
    {
        return USB_FS_RESULT_INVALID_ARG;
    }

    *file_size = 0U;

    if (usb_fs_mounted == 0U)
    {
        return USB_FS_RESULT_NOT_MOUNTED;
    }

    fr = f_stat(path, &finfo);
    if (fr != FR_OK)
    {
        usb_fs_last_error = (uint32_t)fr;
        return ((fr == FR_NO_FILE) || (fr == FR_NO_PATH)) ?
               USB_FS_RESULT_NO_FILE : USB_FS_RESULT_IO_ERROR;
    }

    *file_size = (uint32_t)finfo.fsize;
    return USB_FS_RESULT_OK;
}

uint32_t UsbFsService_GetLastError(void)
{
    return usb_fs_last_error;
}
