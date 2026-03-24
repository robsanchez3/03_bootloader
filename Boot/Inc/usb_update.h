#ifndef USB_UPDATE_H
#define USB_UPDATE_H

#include <stdint.h>
#include "usb_fs_service.h"

#define USB_UPDATE_DIR_PATH       "0:/UPDATE"
#define USB_UPDATE_APP_INI_BIN    "0:/UPDATE/app_ini.bin"
#define USB_UPDATE_APP_OSPI_BIN   "0:/UPDATE/app_ospi.bin"
#define USB_UPDATE_APP_INI_CRC    "0:/UPDATE/app_ini.crc"
#define USB_UPDATE_APP_OSPI_CRC   "0:/UPDATE/app_ospi.crc"

typedef enum
{
    USB_UPDATE_FILE_APP_INI_BIN = 0,
    USB_UPDATE_FILE_APP_OSPI_BIN,
    USB_UPDATE_FILE_APP_INI_CRC,
    USB_UPDATE_FILE_APP_OSPI_CRC,
    USB_UPDATE_FILE_COUNT
} UsbUpdateFileId_t;

typedef struct
{
    const char *path;
    uint8_t present;
} UsbUpdateFileInfo_t;

void UsbUpdate_Init(void);
void UsbUpdate_Process(void);
void UsbUpdate_Reset(void);
uint8_t UsbUpdate_DeviceConnected(void);
uint8_t UsbUpdate_DeviceReady(void);
uint8_t UsbUpdate_FilesPresent(void);
UsbFsResult_t UsbUpdate_ProbeFiles(void);
UsbFsResult_t UsbUpdate_ReadFile(UsbUpdateFileId_t file_id,
                                 void *buffer,
                                 uint32_t offset,
                                 uint32_t bytes_to_read,
                                 uint32_t *bytes_read);
const UsbUpdateFileInfo_t *UsbUpdate_GetFileInfo(UsbUpdateFileId_t file_id);

#endif /* USB_UPDATE_H */
