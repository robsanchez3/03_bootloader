#include "fatfs_usb.h"
#include <stdio.h>

char USBHPath[4];
FATFS USBHFatFS;
FIL USBHFile;
static char usb_file_buffer[64] __attribute__((aligned(32)));

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
    printf("[FATFS] f_mount -> %d\n", res);

    return res;
}

FRESULT USB_FATFS_Unmount(void)
{
    FRESULT res;

    printf("[FATFS] unmount %s\n", USBHPath);
    res = f_mount(NULL, (TCHAR const *)USBHPath, 0);
    printf("[FATFS] f_unmount -> %d\n", res);

    return res;
}

FRESULT USB_FATFS_ReadTestFile(const char *path)
{
    FRESULT res;
    UINT bytes_read;

    printf("[FATFS] open %s\n", path);

    res = f_open(&USBHFile, path, FA_READ);
    printf("[FATFS] f_open -> %d\n", res);
    if (res != FR_OK)
    {
        return res;
    }

    res = f_read(&USBHFile,
                 usb_file_buffer,
                 (UINT)(sizeof(usb_file_buffer) - 1U),
                 &bytes_read);
    printf("[FATFS] f_read -> %d, bytes=%u\n", res, (unsigned)bytes_read);
    if (res == FR_OK)
    {
        usb_file_buffer[bytes_read] = '\0';
        printf("[FATFS] preview: %s\n", usb_file_buffer);
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
