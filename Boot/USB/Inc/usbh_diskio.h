#ifndef USBH_DISKIO_H
#define USBH_DISKIO_H

#include "ff_gen_drv.h"

extern const Diskio_drvTypeDef USBH_Driver;

/* Force BOT + LUN state back to idle so the next SCSI command starts clean. */
void USBH_DiskIO_ResetBotState(void);

#endif /* USBH_DISKIO_H */
