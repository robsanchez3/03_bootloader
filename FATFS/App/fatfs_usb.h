#ifndef FATFS_USB_H
#define FATFS_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "ff_gen_drv.h"
#include "usbh_diskio.h"

extern char USBHPath[4];
extern FATFS USBHFatFS;
extern FIL USBHFile;

void MX_FATFS_USB_Init(void);
FRESULT USB_FATFS_ReadTestFile(const char *path);

#ifdef __cplusplus
}
#endif

#endif