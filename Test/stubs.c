/* Link-time stubs for functions not available on PC */
#include "usb_fs_service.h"

UsbFsResult_t UsbFsService_ReadFile(const char *path,
                                    void *buffer,
                                    uint32_t offset,
                                    uint32_t bytes_to_read,
                                    uint32_t *bytes_read)
{
    (void)path; (void)buffer; (void)offset;
    (void)bytes_to_read; (void)bytes_read;
    return USB_FS_RESULT_NOT_IMPLEMENTED;
}
