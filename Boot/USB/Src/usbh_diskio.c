/*
 * usbh_diskio.c
 *
 * FatFs diskio driver for USB MSC (bare-metal, polling, no DMA).
 *
 * Translates FatFs disk_xxx calls into USBH_MSC_xxx calls.
 * Single-attempt read: on failure, recovery is handled at a higher level
 * (chunked read with full USB host restart).
 *
 * Write is intentionally disabled (RES_WRPRT): the bootloader only
 * reads from the USB drive.
 */

#include "usbh_diskio.h"
#include "usb_msc_service.h"
#include "usbh_msc.h"
#include <stdio.h>

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

    return 0;
}

/* Cap per-BOT-transfer size: FatFs may request very large multi-sector reads
   (a 1 MB chunk over contiguous clusters arrives as a single 2048-sector call,
   i.e. one 1 MB READ(10)). A transient link error anywhere in that burst kills
   the whole transfer. 128 sectors (64 KB @ 512 B) shrinks the exposure window
   ~16x and makes retries cheap. */
#define USBH_READ_MAX_SECTORS    128U

/* Local retries of a failed sub-read before escalating to the caller
   (which triggers the full ForceRestart + re-enumeration recovery). */
#define USBH_READ_LOCAL_RETRIES  3U

static DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    USBH_StatusTypeDef status;
    MSC_HandleTypeDef *msc;
    UINT block_size;

    if (hUsbHostHS.pActiveClass == NULL)
    {
        return RES_NOTRDY;
    }

    if (hUsbHostHS.device.PortEnabled == 0U)
    {
        return RES_NOTRDY;
    }

    msc = (MSC_HandleTypeDef *)hUsbHostHS.pActiveClass->pData;

    block_size = (UINT)msc->unit[lun].capacity.block_size;
    if (block_size == 0U)
    {
        block_size = 512U;
    }

    while (count > 0U)
    {
        UINT n = (count > USBH_READ_MAX_SECTORS) ? USBH_READ_MAX_SECTORS : count;
        UINT attempt;

        for (attempt = 0U; ; attempt++)
        {
            if (msc->unit[lun].state != MSC_IDLE)
            {
                msc->unit[lun].state = MSC_IDLE;
                msc->hbot.state      = BOT_SEND_CBW;
                msc->hbot.cmd_state  = BOT_CMD_SEND;
            }

            status = USBH_MSC_Read(&hUsbHostHS, lun, sector, buff, n);
            if (status == USBH_OK)
            {
                break;
            }

            /* USBH_MSC_LastErr holds the pre-reset state machine snapshot. */
            printf("[DISKIO] t=%lums FAIL sector=%lu n=%u try=%u/%u %s "
                   "unit=%u bot=%u cmd=%u el=%lums PortEnabled=%u\n",
                   (unsigned long)HAL_GetTick(),
                   (unsigned long)sector,
                   (unsigned int)n,
                   (unsigned int)(attempt + 1U),
                   (unsigned int)(USBH_READ_LOCAL_RETRIES + 1U),
                   (USBH_MSC_LastErr.timed_out != 0U) ? "TIMEOUT" : "BOTERR",
                   (unsigned int)USBH_MSC_LastErr.unit_state,
                   (unsigned int)USBH_MSC_LastErr.bot_state,
                   (unsigned int)USBH_MSC_LastErr.cmd_state,
                   (unsigned long)USBH_MSC_LastErr.elapsed_ms,
                   (unsigned int)hUsbHostHS.device.PortEnabled);

            /* HAL-level channel diagnostics: where exactly are the bulk
               pipes stuck? (HC state: 1=XFRC 2=HALTED 3=NAK 5=STALL
               6=XACTERR 7=BBLERR 8=DATATGLERR; URB: 0=IDLE 1=DONE
               2=NOTREADY 3=NYET 4=ERROR 5=STALL — see HCD_HCStateTypeDef
               and HCD_URBStateTypeDef.) */
            {
                HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)hUsbHostHS.pData;
                printf("[DISKIO]   IN pipe %u: hc_state=%u urb=%u xfer=%lu | "
                       "OUT pipe %u: hc_state=%u urb=%u\n",
                       (unsigned int)msc->InPipe,
                       (unsigned int)HAL_HCD_HC_GetState(hhcd, msc->InPipe),
                       (unsigned int)HAL_HCD_HC_GetURBState(hhcd, msc->InPipe),
                       (unsigned long)HAL_HCD_HC_GetXferCount(hhcd, msc->InPipe),
                       (unsigned int)msc->OutPipe,
                       (unsigned int)HAL_HCD_HC_GetState(hhcd, msc->OutPipe),
                       (unsigned int)HAL_HCD_HC_GetURBState(hhcd, msc->OutPipe));
            }

            if (hUsbHostHS.device.PortEnabled == 0U)
            {
                return RES_ERROR;   /* link down — local retry is pointless */
            }
            if (attempt >= USBH_READ_LOCAL_RETRIES)
            {
                return RES_ERROR;   /* escalate to caller's full recovery */
            }

            HAL_Delay(5U);          /* let the drive settle before retrying */
        }

        sector += n;
        buff   += n * block_size;
        count  -= n;
    }

    return RES_OK;
}

static DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    (void)lun;
    (void)buff;
    (void)sector;
    (void)count;

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
