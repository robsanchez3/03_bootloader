#include "fatfs_usb.h"
  #include <stdio.h>

  char USBHPath[4];
  FATFS USBHFatFS;
  FIL USBHFile;
  static char usb_file_buffer[32] __attribute__((aligned(32)));

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

  FRESULT USB_FATFS_ReadTestFile(const char *path)
  {
      FRESULT res;
      UINT bytes_read;

      printf("[FATFS] before f_open: %s\n", path);

      res = f_open(&USBHFile, path, FA_READ);
      printf("[FATFS] after f_open: %d\n", res);
      if (res != FR_OK)
      {
          printf("[FATFS] f_open failed: %d\n", res);
          return res;
      }

      printf("[FATFS] before f_read\n");
      res = f_read(&USBHFile, usb_file_buffer, 16U, &bytes_read);
      printf("[FATFS] after f_read: %d bytes=%u\n", res, (unsigned)bytes_read);
      if (res == FR_OK)
      {
          usb_file_buffer[bytes_read] = '\0';
          printf("[FATFS] Read %u bytes: %s\n", (unsigned)bytes_read, usb_file_buffer);
      }
      else
      {
          printf("[FATFS] f_read failed: %d\n", res);
      }

      f_close(&USBHFile);
      return res;
  }
