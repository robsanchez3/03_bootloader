#ifndef USB_FS_SERVICE_H
#define USB_FS_SERVICE_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
    USB_FS_RESULT_OK = 0,
    USB_FS_RESULT_NOT_READY,
    USB_FS_RESULT_NOT_MOUNTED,
    USB_FS_RESULT_NO_FILE,
    USB_FS_RESULT_IO_ERROR,
    USB_FS_RESULT_INVALID_ARG,
    USB_FS_RESULT_NOT_IMPLEMENTED
} UsbFsResult_t;

void UsbFsService_Init(void);
void UsbFsService_Process(void);
void UsbFsService_Reset(void);
uint8_t UsbFsService_IsMounted(void);
UsbFsResult_t UsbFsService_Mount(void);
UsbFsResult_t UsbFsService_Unmount(void);
UsbFsResult_t UsbFsService_FileExists(const char *path);
UsbFsResult_t UsbFsService_ReadFile(const char *path,
                                    void *buffer,
                                    uint32_t offset,
                                    uint32_t bytes_to_read,
                                    uint32_t *bytes_read);

/* Streaming API para lectura secuencial de ficheros grandes.
 * Solo un stream abierto a la vez. */
UsbFsResult_t UsbFsService_OpenStream (const char *path);
UsbFsResult_t UsbFsService_ReadStream (void *buffer,
                                       uint32_t bytes_to_read,
                                       uint32_t *bytes_read);
UsbFsResult_t UsbFsService_CloseStream(void);

uint32_t UsbFsService_GetLastError(void);

#endif /* USB_FS_SERVICE_H */
