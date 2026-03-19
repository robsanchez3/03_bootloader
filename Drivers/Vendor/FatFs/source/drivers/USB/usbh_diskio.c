/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    FATFS/Target/usbh_diskio.c
  * @brief   USB Host Disk I/O driver for FatFS - STM32U5 adaptation
  *
  * Adapted from OztDUI_UsbFlash reference project.
  * Key change: hUSB_Host -> hUsbHostHS (USB_OTG_HS handle in usb_host.c)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "usbh_core.h"
#include "usbh_msc.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define USB_DEFAULT_BLOCK_SIZE  512U
#define USB_READ_RETRIES       12U
#define USB_READ_RETRY_DELAY_MS 25U

/* Private variables ---------------------------------------------------------*/
/* hUsbHostHS is declared in USB_Host/App/usb_host.c */
extern USBH_HandleTypeDef  hUsbHostHS;

/* Use a fixed aligned bounce buffer for MSC reads triggered by FatFS. */
static uint8_t usbh_sector_buf[USB_DEFAULT_BLOCK_SIZE] __attribute__((aligned(32)));

/* Private function prototypes -----------------------------------------------*/
DSTATUS USBH_initialize(BYTE lun);
DSTATUS USBH_status(BYTE lun);
DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count);

#if _USE_WRITE == 1
  DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count);
#endif

#if _USE_IOCTL == 1
  DRESULT USBH_ioctl(BYTE lun, BYTE cmd, void *buff);
#endif

/* FatFS driver interface registration */
const Diskio_drvTypeDef USBH_Driver =
{
  USBH_initialize,
  USBH_status,
  USBH_read,
#if _USE_WRITE == 1
  USBH_write,
#endif
#if _USE_IOCTL == 1
  USBH_ioctl,
#endif
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initialize USB drive
  */
DSTATUS USBH_initialize(BYTE lun)
{
  /* USB Host library must be initialized in the application before FatFS mount */
  return RES_OK;
}

/**
  * @brief  Get USB drive status
  */
DSTATUS USBH_status(BYTE lun)
{
  if ((hUsbHostHS.pActiveClass == NULL) || (hUsbHostHS.pActiveClass->pData == NULL))
  {
    return RES_ERROR;
  }

  if (USBH_MSC_UnitIsReady(&hUsbHostHS, lun))
    return RES_OK;
  else
    return RES_ERROR;
}

/**
  * @brief  Read sector(s)
  */
DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;
  MSC_LUNTypeDef info;
  UINT i;
  UINT attempt;

  for (i = 0; i < count; i++)
  {
    res = RES_ERROR;

    for (attempt = 0; attempt < USB_READ_RETRIES; attempt++)
    {
      if (USBH_MSC_UnitIsReady(&hUsbHostHS, lun) == 0U)
      {
        if (attempt + 1U < USB_READ_RETRIES)
        {
          HAL_Delay(USB_READ_RETRY_DELAY_MS);
        }
        continue;
      }

      if (USBH_MSC_Read(&hUsbHostHS, lun, sector + i, usbh_sector_buf, 1U) == USBH_OK)
      {
        memcpy(buff + (i * USB_DEFAULT_BLOCK_SIZE), usbh_sector_buf, USB_DEFAULT_BLOCK_SIZE);
        res = RES_OK;
        break;
      }

      if (attempt + 1U < USB_READ_RETRIES)
      {
        HAL_Delay(USB_READ_RETRY_DELAY_MS);
      }
    }

    if (res == RES_OK)
    {
      continue;
    }

    USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info);
    switch (info.sense.asc)
    {
      case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
      case SCSI_ASC_MEDIUM_NOT_PRESENT:
      case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
        USBH_ErrLog("USB Disk is not ready!");
        res = RES_NOTRDY;
        break;
      default:
        res = RES_ERROR;
        break;
    }
    break;
  }
  return res;
}

#if _USE_WRITE == 1
/**
  * @brief  Write sector(s)
  */
DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;
  MSC_LUNTypeDef info;

  if (USBH_MSC_Write(&hUsbHostHS, lun, sector, (BYTE *)buff, count) == USBH_OK)
  {
    res = RES_OK;
  }
  else
  {
    USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info);
    switch (info.sense.asc)
    {
      case SCSI_ASC_WRITE_PROTECTED:
        USBH_ErrLog("USB Disk is Write protected!");
        res = RES_WRPRT;
        break;
      case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
      case SCSI_ASC_MEDIUM_NOT_PRESENT:
      case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
        USBH_ErrLog("USB Disk is not ready!");
        res = RES_NOTRDY;
        break;
      default:
        res = RES_ERROR;
        break;
    }
  }
  return res;
}
#endif /* _USE_WRITE */

#if _USE_IOCTL == 1
/**
  * @brief  IOCTL - get drive info
  */
DRESULT USBH_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;
  MSC_LUNTypeDef info;

  switch (cmd)
  {
    case CTRL_SYNC:
      res = RES_OK;
      break;

    case GET_SECTOR_COUNT:
      if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK)
      {
        *(DWORD *)buff = info.capacity.block_nbr;
        res = RES_OK;
      }
      break;

    case GET_SECTOR_SIZE:
      if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK)
      {
        *(DWORD *)buff = info.capacity.block_size;
        res = RES_OK;
      }
      break;

    case GET_BLOCK_SIZE:
      if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK)
      {
        *(DWORD *)buff = info.capacity.block_size / USB_DEFAULT_BLOCK_SIZE;
        res = RES_OK;
      }
      break;

    default:
      res = RES_PARERR;
      break;
  }
  return res;
}
#endif /* _USE_IOCTL */
