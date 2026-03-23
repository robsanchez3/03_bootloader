#include "fatfs_usb.h"
#include <stdio.h>

char USBHPath[4];
FATFS USBHFatFS;
FIL USBHFile;
static uint8_t usb_file_buffer[256] __attribute__((aligned(32)));

static const char *USB_FATFS_ResultString(FRESULT res)
{
    switch (res)
    {
        case FR_OK: return "FR_OK";
        case FR_DISK_ERR: return "FR_DISK_ERR";
        case FR_INT_ERR: return "FR_INT_ERR";
        case FR_NOT_READY: return "FR_NOT_READY";
        case FR_NO_FILE: return "FR_NO_FILE";
        case FR_NO_PATH: return "FR_NO_PATH";
        case FR_INVALID_NAME: return "FR_INVALID_NAME";
        case FR_DENIED: return "FR_DENIED";
        case FR_EXIST: return "FR_EXIST";
        case FR_INVALID_OBJECT: return "FR_INVALID_OBJECT";
        case FR_WRITE_PROTECTED: return "FR_WRITE_PROTECTED";
        case FR_INVALID_DRIVE: return "FR_INVALID_DRIVE";
        case FR_NOT_ENABLED: return "FR_NOT_ENABLED";
        case FR_NO_FILESYSTEM: return "FR_NO_FILESYSTEM";
        case FR_MKFS_ABORTED: return "FR_MKFS_ABORTED";
        case FR_TIMEOUT: return "FR_TIMEOUT";
        case FR_LOCKED: return "FR_LOCKED";
        case FR_NOT_ENOUGH_CORE: return "FR_NOT_ENOUGH_CORE";
        case FR_TOO_MANY_OPEN_FILES: return "FR_TOO_MANY_OPEN_FILES";
        case FR_INVALID_PARAMETER: return "FR_INVALID_PARAMETER";
        default: return "FR_UNKNOWN";
    }
}

static uint32_t USB_FATFS_Crc32Update(uint32_t crc, const uint8_t *data, UINT length)
{
    UINT index;
    uint32_t bit;

    for (index = 0U; index < length; index++)
    {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

void MX_FATFS_USB_Init(void)
{
    if (FATFS_LinkDriver(&USBH_Driver, USBHPath) == 0)
    {
        printf("[FATFS] USB driver linked: %s\n", USBHPath);
    }
    else
    {
        printf("[FATFS] USB driver link failed\n");
    }
}

FRESULT USB_FATFS_Mount(void)
{
    FRESULT res;

    printf("[FATFS] mount %s\n", USBHPath);
    res = f_mount(&USBHFatFS, (TCHAR const *)USBHPath, 1);
    printf("[FATFS] f_mount -> %d (%s)\n", res, USB_FATFS_ResultString(res));

    return res;
}

FRESULT USB_FATFS_Unmount(void)
{
    FRESULT res;

    printf("[FATFS] unmount %s\n", USBHPath);
    res = f_mount(NULL, (TCHAR const *)USBHPath, 0);
    printf("[FATFS] f_unmount -> %d (%s)\n", res, USB_FATFS_ResultString(res));

    return res;
}

FRESULT USB_FATFS_GetFileSize(const char *path, FSIZE_t *size_out)
{
    FRESULT res;
    FILINFO file_info;

    printf("[FATFS] stat %s\n", path);
    res = f_stat(path, &file_info);
    printf("[FATFS] f_stat -> %d (%s)\n", res, USB_FATFS_ResultString(res));
    if ((res == FR_OK) && (size_out != NULL))
    {
        *size_out = file_info.fsize;
        printf("[FATFS] size=%lu bytes\n", (unsigned long)file_info.fsize);
    }

    return res;
}

FRESULT USB_FATFS_ReadTextFile(const char *path, char *buffer, UINT buffer_len)
{
    FRESULT res;
    UINT bytes_read;

    if ((buffer == NULL) || (buffer_len < 2U))
    {
        return FR_INVALID_PARAMETER;
    }

    printf("[FATFS] open %s\n", path);

    res = f_open(&USBHFile, path, FA_READ);
    printf("[FATFS] f_open -> %d (%s)\n", res, USB_FATFS_ResultString(res));
    if (res != FR_OK)
    {
        return res;
    }

    res = f_read(&USBHFile,
                 buffer,
                 (UINT)(buffer_len - 1U),
                 &bytes_read);
    printf("[FATFS] f_read -> %d (%s), bytes=%u\n",
           res,
           USB_FATFS_ResultString(res),
           (unsigned)bytes_read);
    if (res == FR_OK)
    {
        buffer[bytes_read] = '\0';
        printf("[FATFS] preview: %s\n", buffer);
    }
    else
    {
        printf("[FATFS] f_read failed: %d\n", res);
    }

    if (f_close(&USBHFile) != FR_OK)
    {
        printf("[FATFS] f_close failed\n");
    }

    return res;
}

FRESULT USB_FATFS_ReadFilePreview(const char *path, UINT preview_len)
{
    FRESULT res;
    UINT bytes_read;
    UINT read_len = preview_len;

    if (read_len > sizeof(usb_file_buffer))
    {
        read_len = sizeof(usb_file_buffer);
    }

    printf("[FATFS] preview open %s\n", path);
    res = f_open(&USBHFile, path, FA_READ);
    printf("[FATFS] preview f_open -> %d (%s)\n", res, USB_FATFS_ResultString(res));
    if (res != FR_OK)
    {
        return res;
    }

    res = f_read(&USBHFile, usb_file_buffer, read_len, &bytes_read);
    printf("[FATFS] preview f_read -> %d (%s), bytes=%u\n",
           res,
           USB_FATFS_ResultString(res),
           (unsigned)bytes_read);

    if (f_close(&USBHFile) != FR_OK)
    {
        printf("[FATFS] preview f_close failed\n");
    }

    return res;
}

FRESULT USB_FATFS_ComputeFileCrc32(const char *path, uint32_t *crc_out, FSIZE_t *size_out)
{
    FRESULT res;
    UINT bytes_read;
    uint32_t crc = 0xFFFFFFFFU;
    FSIZE_t total_size = 0U;

    if (crc_out == NULL)
    {
        return FR_INVALID_PARAMETER;
    }

    printf("[FATFS] open %s for CRC\n", path);
    res = f_open(&USBHFile, path, FA_READ);
    printf("[FATFS] f_open -> %d (%s)\n", res, USB_FATFS_ResultString(res));
    if (res != FR_OK)
    {
        return res;
    }

    do
    {
        res = f_read(&USBHFile, usb_file_buffer, sizeof(usb_file_buffer), &bytes_read);
        if (res != FR_OK)
        {
            printf("[FATFS] f_read CRC -> %d (%s)\n", res, USB_FATFS_ResultString(res));
            (void)f_close(&USBHFile);
            return res;
        }

        crc = USB_FATFS_Crc32Update(crc, usb_file_buffer, bytes_read);
        total_size += (FSIZE_t)bytes_read;
    } while (bytes_read > 0U);

    if (f_close(&USBHFile) != FR_OK)
    {
        printf("[FATFS] f_close failed after CRC\n");
    }

    *crc_out = crc ^ 0xFFFFFFFFU;
    if (size_out != NULL)
    {
        *size_out = total_size;
    }

    printf("[FATFS] CRC32(%s) = 0x%08lX size=%lu\n",
           path,
           (unsigned long)(*crc_out),
           (unsigned long)total_size);

    return FR_OK;
}
