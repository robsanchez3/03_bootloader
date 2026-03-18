#include "fatfs_usb.h"
  #include <stdio.h>

  char USBHPath[4];
  FATFS USBHFatFS;
  FIL USBHFile;

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
      char buffer[128];

      res = f_open(&USBHFile, path, FA_READ);
      if (res != FR_OK)
      {
          printf("[FATFS] f_open failed: %d\n", res);
          return res;
      }

      res = f_read(&USBHFile, buffer, sizeof(buffer) - 1, &bytes_read);
      if (res == FR_OK)
      {
          buffer[bytes_read] = '\0';
          printf("[FATFS] Read %u bytes: %s\n", (unsigned)bytes_read, buffer);
      }
      else
      {
          printf("[FATFS] f_read failed: %d\n", res);
      }

      f_close(&USBHFile);
      return res;
  }
