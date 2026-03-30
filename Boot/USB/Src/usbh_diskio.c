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
    uint8_t retry;
    for (retry = 0U; retry < 3U; retry++)
    {
        if (hUsbHostHS.pActiveClass == NULL)
        {
            return RES_NOTRDY;
        }

        /* No tiene sentido reintentar si el puerto sigue sin estar habilitado. */
        if (hUsbHostHS.device.PortEnabled == 0U)
        {
            return RES_NOTRDY;
        }

        msc = (MSC_HandleTypeDef *)hUsbHostHS.pActiveClass->pData;

        /* Cuando USBH_MSC_Read falla porque PortEnabled cae a 0 dentro de su
           bucle interno, deja dos estados inconsistentes:
             - unit[lun].state = MSC_READ (6)   — la lectura nunca terminó
             - hbot.cmd_state  = BOT_CMD_WAIT    — el SCSI_Read ya emitió el CBW
             - hbot.state      = estado intermedio del BOT
           Si solo reseteamos unit.state y NO el BOT, USBH_MSC_SCSI_Read entra
           por el case BOT_CMD_WAIT en lugar de BOT_CMD_SEND y no prepara el CBW
           nuevo, fallando de inmediato.  Resetear los tres campos reinicia la
           cadena BOT desde cero, igual que hace USBH_MSC_BOT_Init. */
        if (msc->unit[lun].state != MSC_IDLE)
        {
            printf("[DISKIO] sector=%lu retry=%u: lun=%u bot=%u cmd=%u -> reset BOT\n",
                   (unsigned long)sector, (unsigned int)retry,
                   (unsigned int)msc->unit[lun].state,
                   (unsigned int)msc->hbot.state,
                   (unsigned int)msc->hbot.cmd_state);

            msc->unit[lun].state = MSC_IDLE;
            msc->hbot.state      = BOT_SEND_CBW;
            msc->hbot.cmd_state  = BOT_CMD_SEND;
            HAL_Delay(10U);
        }

        status = USBH_MSC_Read(&hUsbHostHS, lun, sector, buff, count);

        if (status == USBH_OK)
        {
            return RES_OK;
        }

        printf("[DISKIO] FAIL sector=%lu retry=%u lun=%u bot=%u cmd=%u PortEnabled=%u\n",
               (unsigned long)sector, (unsigned int)retry,
               (unsigned int)msc->unit[lun].state,
               (unsigned int)msc->hbot.state,
               (unsigned int)msc->hbot.cmd_state,
               (unsigned int)hUsbHostHS.device.PortEnabled);

        HAL_Delay(50U);
    }

    return RES_ERROR;
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
