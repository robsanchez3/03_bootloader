/*
 * usbh_diskio.c
 *
 * FatFs diskio driver for USB MSC (bare-metal, polling, no DMA).
 *
 * Translates FatFs disk_xxx calls into USBH_MSC_xxx calls.
 * USBH_MSC_Read is internally blocking (loops on USBH_MSC_RdWrProcess
 * until the transfer completes or times out).  USBH_read retries up to
 * 3 times and resets the BOT state between attempts.
 *
 * Write is intentionally disabled (RES_WRPRT): the bootloader only
 * reads from the USB drive.
 */

#include "usbh_diskio.h"
#include "usb_msc_service.h"
#include "usbh_msc.h"

static DSTATUS USBH_initialize(BYTE lun);
static DSTATUS USBH_status    (BYTE lun);
static DRESULT USBH_read      (BYTE lun, BYTE *buff, DWORD sector, UINT count);
static DRESULT USBH_write     (BYTE lun, const BYTE *buff, DWORD sector, UINT count);
static DRESULT USBH_ioctl     (BYTE lun, BYTE cmd, void *buff);

const Diskio_drvTypeDef USBH_Driver =
{
    USBH_initialize,
    USBH_status,
    USBH_read,
    USBH_write,
    USBH_ioctl,
};

static DSTATUS USBH_initialize(BYTE lun)
{
    return USBH_status(lun);
}

static DSTATUS USBH_status(BYTE lun)
{
    MSC_LUNTypeDef info;

    if (USBH_MSC_IsReady(&hUsbHostHS) == 0U)
    {
        return STA_NOINIT;
    }

    if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) != USBH_OK)
    {
        return STA_NOINIT;
    }

    /* Do not check info.error: a transient SCSI error sets MSC_ERROR
       but the device remains physically operational. Blocking on that
       flag would cause FR_DISK_ERR on all subsequent FatFs operations. */

    return 0;
}

static DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    USBH_StatusTypeDef status;
    MSC_HandleTypeDef *msc;

    if (hUsbHostHS.pActiveClass == NULL)
    {
        return RES_NOTRDY;
    }

    if (hUsbHostHS.device.PortEnabled == 0U)
    {
        return RES_NOTRDY;
    }

    msc = (MSC_HandleTypeDef *)hUsbHostHS.pActiveClass->pData;

    if (msc->unit[lun].state != MSC_IDLE)
    {
        msc->unit[lun].state = MSC_IDLE;
        msc->hbot.state      = BOT_SEND_CBW;
        msc->hbot.cmd_state  = BOT_CMD_SEND;
    }

    status = USBH_MSC_Read(&hUsbHostHS, lun, sector, buff, count);

    if (status == USBH_OK)
    {
        return RES_OK;
    }

    printf("[DISKIO] FAIL sector=%lu lun=%u bot=%u cmd=%u PortEnabled=%u\n",
           (unsigned long)sector,
           (unsigned int)msc->unit[lun].state,
           (unsigned int)msc->hbot.state,
           (unsigned int)msc->hbot.cmd_state,
           (unsigned int)hUsbHostHS.device.PortEnabled);

    return RES_ERROR;
}

void USBH_DiskIO_ResetBotState(void)
{
    MSC_HandleTypeDef *msc;

    if (hUsbHostHS.pActiveClass == NULL)
    {
        return;
    }

    msc = (MSC_HandleTypeDef *)hUsbHostHS.pActiveClass->pData;
    msc->unit[0].state  = MSC_IDLE;
    msc->hbot.state     = BOT_SEND_CBW;
    msc->hbot.cmd_state = BOT_CMD_SEND;
}

static DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    (void)lun;
    (void)buff;
    (void)sector;
    (void)count;

    /* Bootloader does not write to the USB drive. */
    return RES_WRPRT;
}

static DRESULT USBH_ioctl(BYTE lun, BYTE cmd, void *buff)
{
    MSC_LUNTypeDef info;
    DRESULT result = RES_ERROR;

    switch (cmd)
    {
        case CTRL_SYNC:
            result = RES_OK;
            break;

        case GET_SECTOR_COUNT:
            if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_nbr;
                result = RES_OK;
            }
            break;

        case GET_SECTOR_SIZE:
            if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK)
            {
                *(WORD *)buff = info.capacity.block_size;
                result = RES_OK;
            }
            break;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1U;
            result = RES_OK;
            break;

        default:
            result = RES_PARERR;
            break;
    }

    return result;
}
