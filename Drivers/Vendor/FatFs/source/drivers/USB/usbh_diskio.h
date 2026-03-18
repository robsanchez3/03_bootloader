/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    FATFS/Target/usbh_diskio.h
  * @brief   Header for usbh_diskio.c - USB Host FatFS disk I/O driver
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USBH_DISKIO_H
#define __USBH_DISKIO_H

/* Includes ------------------------------------------------------------------*/
#include "ff_gen_drv.h"

/* Exported functions ------------------------------------------------------- */
extern const Diskio_drvTypeDef  USBH_Driver;

#endif /* __USBH_DISKIO_H */
