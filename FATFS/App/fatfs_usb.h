#ifndef FATFS_USB_H
#define FATFS_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include <stdint.h>

extern char USBHPath[4];
extern FATFS USBHFatFS;
extern FIL USBHFile;

void MX_FATFS_USB_Init(void);
FRESULT USB_FATFS_Mount(void);
FRESULT USB_FATFS_Unmount(void);
FRESULT USB_FATFS_GetFileSize(const char *path, FSIZE_t *size_out);
FRESULT USB_FATFS_ReadTextFile(const char *path, char *buffer, UINT buffer_len);
FRESULT USB_FATFS_ReadFilePreview(const char *path, UINT preview_len);
FRESULT USB_FATFS_ComputeFileCrc32(const char *path, uint32_t *crc_out, FSIZE_t *size_out);

#ifdef __cplusplus
}
#endif

#endif
