/**
  ******************************************************************************
  * @file    usbh_msc.c
  * @author  MCD Application Team
  * @brief   This file implements the MSC class driver functions
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *  @verbatim
  *
  *          ===================================================================
  *                                MSC Class  Description
  *          ===================================================================
  *           This module manages the MSC class V1.0 following the "Universal
  *           Serial Bus Mass Storage Class (MSC) Bulk-Only Transport (BOT) Version 1.0
  *           Sep. 31, 1999".
  *           This driver implements the following aspects of the specification:
  *             - Bulk-Only Transport protocol
  *             - Subclass : SCSI transparent command set (ref. SCSI Primary Commands - 3 (SPC-3))
  *
  *  @endverbatim
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
- "stm32xxxxx_{eval}{discovery}{adafruit}_lcd.c"
- "stm32xxxxx_{eval}{discovery}_sdram.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbh_msc.h"
#include "usbh_msc_bot.h"
#include "usbh_msc_scsi.h"
#include "stm32u5xx_hal_hcd.h"
#include <stdio.h>

#define MSC_INQUIRY_TIMEOUT_MS        5000U
#define MSC_READ_CAPACITY_TIMEOUT_MS  5000U
#define MSC_REQUEST_SENSE_TIMEOUT_MS  5000U

typedef struct
{
  uint32_t signature;
  uint32_t timer_now;
  uint32_t timer_start;
  uint32_t address;
  uint32_t length;
  uint32_t host_gstate;
  uint32_t port_enabled;
  uint32_t current_lun;
  uint32_t unit_state;
  uint32_t unit_error;
  uint32_t bot_state;
  uint32_t bot_cmd_state;
  uint32_t in_pipe;
  uint32_t out_pipe;
  uint32_t hhcd_state;
  uint32_t hhcd_error_code;
  uint32_t in_urb_state;
  uint32_t out_urb_state;
  uint32_t in_hc_state;
  uint32_t out_hc_state;
  uint32_t in_err_cnt;
  uint32_t out_err_cnt;
  uint32_t in_hcchar;
  uint32_t in_hcint;
  uint32_t in_hcintmsk;
  uint32_t in_hctsiz;
  uint32_t out_hcchar;
  uint32_t out_hcint;
  uint32_t out_hcintmsk;
  uint32_t out_hctsiz;
} MSC_ReadTimeoutSnapshot;

volatile MSC_ReadTimeoutSnapshot g_msc_read_timeout_snapshot;


/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_MSC_CLASS
  * @{
  */

/** @defgroup USBH_MSC_CORE
  * @brief    This file includes the mass storage related functions
  * @{
  */

/** @defgroup USBH_MSC_CORE_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_MSC_CORE_Private_Defines
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_MSC_CORE_Private_Macros
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_MSC_CORE_Private_Variables
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_MSC_CORE_Private_FunctionPrototypes
  * @{
  */

static USBH_StatusTypeDef USBH_MSC_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MSC_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MSC_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MSC_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MSC_SOFProcess(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MSC_RdWrProcess(USBH_HandleTypeDef *phost, uint8_t lun);
static const char *USBH_MSC_StateName(uint8_t state);

USBH_ClassTypeDef  USBH_msc =
{
  "MSC",
  USB_MSC_CLASS,
  USBH_MSC_InterfaceInit,
  USBH_MSC_InterfaceDeInit,
  USBH_MSC_ClassRequest,
  USBH_MSC_Process,
  USBH_MSC_SOFProcess,
  NULL,
};

/**
  * @}
  */

/** @defgroup USBH_MSC_CORE_Exported_Variables
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_MSC_CORE_Private_Functions
  * @{
  */

/**
  * @brief  USBH_MSC_InterfaceInit
  *         The function init the MSC class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MSC_InterfaceInit(USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status;
  uint8_t interface;
  MSC_HandleTypeDef *MSC_Handle;

  interface = USBH_FindInterface(phost, phost->pActiveClass->ClassCode, MSC_TRANSPARENT, MSC_BOT);

  if ((interface == 0xFFU) || (interface >= USBH_MAX_NUM_INTERFACES)) /* Not Valid Interface */
  {
    USBH_DbgLog("Cannot Find the interface for %s class.", phost->pActiveClass->Name);
    return USBH_FAIL;
  }

  status = USBH_SelectInterface(phost, interface);

  if (status != USBH_OK)
  {
    return USBH_FAIL;
  }

  phost->pActiveClass->pData = (MSC_HandleTypeDef *)USBH_malloc(sizeof(MSC_HandleTypeDef));
  MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;

  if (MSC_Handle == NULL)
  {
    USBH_DbgLog("Cannot allocate memory for MSC Handle");
    return USBH_FAIL;
  }

  /* Initialize msc handler */
  (void)USBH_memset(MSC_Handle, 0, sizeof(MSC_HandleTypeDef));

  if ((phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress & 0x80U) != 0U)
  {
    MSC_Handle->InEp = (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress);
    MSC_Handle->InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
  }
  else
  {
    MSC_Handle->OutEp = (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress);
    MSC_Handle->OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
  }

  if ((phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress & 0x80U) != 0U)
  {
    MSC_Handle->InEp = (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress);
    MSC_Handle->InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
  }
  else
  {
    MSC_Handle->OutEp = (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress);
    MSC_Handle->OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
  }

  MSC_Handle->state = MSC_INIT;
  MSC_Handle->error = MSC_OK;
  MSC_Handle->req_state = MSC_REQ_IDLE;
  MSC_Handle->OutPipe = USBH_AllocPipe(phost, MSC_Handle->OutEp);
  MSC_Handle->InPipe = USBH_AllocPipe(phost, MSC_Handle->InEp);

  (void)USBH_MSC_BOT_Init(phost);

  /* Open the new channels */
  if ((MSC_Handle->OutEp != 0U) && (MSC_Handle->OutEpSize != 0U))
  {
    (void)USBH_OpenPipe(phost, MSC_Handle->OutPipe, MSC_Handle->OutEp,
                        phost->device.address, phost->device.speed,
                        USB_EP_TYPE_BULK, MSC_Handle->OutEpSize);
  }
  else
  {
    return USBH_NOT_SUPPORTED;
  }

  if ((MSC_Handle->InEp != 0U) && (MSC_Handle->InEpSize != 0U))
  {
    (void)USBH_OpenPipe(phost, MSC_Handle->InPipe, MSC_Handle->InEp,
                        phost->device.address, phost->device.speed, USB_EP_TYPE_BULK,
                        MSC_Handle->InEpSize);
  }
  else
  {
    return USBH_NOT_SUPPORTED;
  }

  (void)USBH_LL_SetToggle(phost, MSC_Handle->InPipe, 0U);
  (void)USBH_LL_SetToggle(phost, MSC_Handle->OutPipe, 0U);

  return USBH_OK;
}

/**
  * @brief  USBH_MSC_InterfaceDeInit
  *         The function DeInit the Pipes used for the MSC class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MSC_InterfaceDeInit(USBH_HandleTypeDef *phost)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;

  if ((MSC_Handle->OutPipe) != 0U)
  {
    (void)USBH_ClosePipe(phost, MSC_Handle->OutPipe);
    (void)USBH_FreePipe(phost, MSC_Handle->OutPipe);
    MSC_Handle->OutPipe = 0U;     /* Reset the Channel as Free */
  }

  if ((MSC_Handle->InPipe != 0U))
  {
    (void)USBH_ClosePipe(phost, MSC_Handle->InPipe);
    (void)USBH_FreePipe(phost, MSC_Handle->InPipe);
    MSC_Handle->InPipe = 0U;     /* Reset the Channel as Free */
  }

  if ((phost->pActiveClass->pData) != NULL)
  {
    USBH_free(phost->pActiveClass->pData);
    phost->pActiveClass->pData = 0U;
  }

  return USBH_OK;
}

/**
  * @brief  USBH_MSC_ClassRequest
  *         The function is responsible for handling Standard requests
  *         for MSC class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MSC_ClassRequest(USBH_HandleTypeDef *phost)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_BUSY;
  uint8_t lun_idx;

  /* Switch MSC REQ state machine */
  switch (MSC_Handle->req_state)
  {
    case MSC_REQ_IDLE:
    case MSC_REQ_GET_MAX_LUN:

      /* Issue GetMaxLUN request */
      status = USBH_MSC_BOT_REQ_GetMaxLUN(phost, &MSC_Handle->max_lun);

      /* When devices do not support the GetMaxLun request, this should
         be considered as only one logical unit is supported */
      if (status == USBH_NOT_SUPPORTED)
      {
        MSC_Handle->max_lun = 0U;
        status = USBH_OK;
      }

      if (status == USBH_OK)
      {
        MSC_Handle->max_lun = (MSC_Handle->max_lun > MAX_SUPPORTED_LUN) ? MAX_SUPPORTED_LUN : (MSC_Handle->max_lun + 1U);
        USBH_UsrLog("Number of supported LUN: %d", MSC_Handle->max_lun);

        for (lun_idx = 0U; lun_idx < MSC_Handle->max_lun; lun_idx++)
        {
          MSC_Handle->unit[lun_idx].prev_ready_state = USBH_FAIL;
          MSC_Handle->unit[lun_idx].state_changed = 0U;
        }
      }
      break;

    case MSC_REQ_ERROR:
      /* a Clear Feature should be issued here */
      if (USBH_ClrFeature(phost, 0x00U) == USBH_OK)
      {
        MSC_Handle->req_state = MSC_Handle->prev_req_state;
      }
      break;

    default:
      break;
  }

  return status;
}

/**
  * @brief  USBH_MSC_Process
  *         The function is for managing state machine for MSC data transfers
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MSC_Process(USBH_HandleTypeDef *phost)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;
  USBH_StatusTypeDef error = USBH_BUSY;
  USBH_StatusTypeDef scsi_status = USBH_BUSY;
  USBH_StatusTypeDef ready_status = USBH_BUSY;
  static uint8_t last_msc_state = 0xFFU;
  static uint8_t last_lun_state = 0xFFU;
  static uint8_t last_lun_index = 0xFFU;

  if (MSC_Handle->state != last_msc_state)
  {
    last_msc_state = MSC_Handle->state;
    USBH_UsrLog("[MSC] core state -> %s", USBH_MSC_StateName(MSC_Handle->state));
  }

  if ((MSC_Handle->current_lun < MSC_Handle->max_lun) &&
      ((MSC_Handle->current_lun != last_lun_index) ||
       (MSC_Handle->unit[MSC_Handle->current_lun].state != last_lun_state)))
  {
    last_lun_index = MSC_Handle->current_lun;
    last_lun_state = MSC_Handle->unit[MSC_Handle->current_lun].state;
    USBH_UsrLog("[MSC] lun %u state -> %s",
                (unsigned int)MSC_Handle->current_lun,
                USBH_MSC_StateName(MSC_Handle->unit[MSC_Handle->current_lun].state));
  }

  switch (MSC_Handle->state)
  {
    case MSC_INIT:

      if (MSC_Handle->current_lun < MSC_Handle->max_lun)
      {
        MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_NOT_READY;

        /* Switch MSC REQ state machine */
        switch (MSC_Handle->unit[MSC_Handle->current_lun].state)
        {
          case MSC_INIT:
            USBH_UsrLog("LUN #%d: ", MSC_Handle->current_lun);
            MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_READ_INQUIRY;
            MSC_Handle->timer = phost->Timer;
            break;

          case MSC_READ_INQUIRY:
            scsi_status = USBH_MSC_SCSI_Inquiry(phost, (uint8_t)MSC_Handle->current_lun, &MSC_Handle->unit[MSC_Handle->current_lun].inquiry);

            if (scsi_status == USBH_OK)
            {
              USBH_UsrLog("Inquiry Vendor  : %s", MSC_Handle->unit[MSC_Handle->current_lun].inquiry.vendor_id);
              USBH_UsrLog("Inquiry Product : %s", MSC_Handle->unit[MSC_Handle->current_lun].inquiry.product_id);
              USBH_UsrLog("Inquiry Version : %s", MSC_Handle->unit[MSC_Handle->current_lun].inquiry.revision_id);
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_TEST_UNIT_READY;
              MSC_Handle->timer = phost->Timer;
            }
            else if (scsi_status == USBH_FAIL)
            {
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_REQUEST_SENSE;
            }
            else
            {
              if ((phost->Timer - MSC_Handle->timer) >= MSC_INQUIRY_TIMEOUT_MS)
              {
                USBH_UsrLog("[MSC] Inquiry timeout");
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_REQUEST_SENSE;
                if (phost->pUser != NULL)
                {
                  phost->pUser(phost, HOST_USER_UNRECOVERED_ERROR);
                }
              }

              if (scsi_status == USBH_UNRECOVERED_ERROR)
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_UNRECOVERED_ERROR;
                MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_ERROR;
              }
            }
            break;

          case MSC_TEST_UNIT_READY:
            ready_status = USBH_MSC_SCSI_TestUnitReady(phost, (uint8_t)MSC_Handle->current_lun);

            if (ready_status == USBH_OK)
            {
              if (MSC_Handle->unit[MSC_Handle->current_lun].prev_ready_state != USBH_OK)
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state_changed = 1U;
                USBH_UsrLog("MSC Device ready");
              }
              else
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state_changed = 0U;
              }
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_READ_CAPACITY10;
              MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_OK;
              MSC_Handle->unit[MSC_Handle->current_lun].prev_ready_state = USBH_OK;
              MSC_Handle->timer = phost->Timer;
            }
            else if (ready_status == USBH_FAIL)
            {
              /* Media not ready, so try to check again during 10s */
              if (MSC_Handle->unit[MSC_Handle->current_lun].prev_ready_state != USBH_FAIL)
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state_changed = 1U;
                USBH_UsrLog("MSC Device NOT ready");
              }
              else
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state_changed = 0U;
              }
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_REQUEST_SENSE;
              MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_NOT_READY;
              MSC_Handle->unit[MSC_Handle->current_lun].prev_ready_state = USBH_FAIL;
            }
            else
            {
              if (ready_status == USBH_UNRECOVERED_ERROR)
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_UNRECOVERED_ERROR;
                MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_ERROR;
              }
            }
            break;

          case MSC_READ_CAPACITY10:
            scsi_status = USBH_MSC_SCSI_ReadCapacity(phost, (uint8_t)MSC_Handle->current_lun, &MSC_Handle->unit[MSC_Handle->current_lun].capacity);

            if (scsi_status == USBH_OK)
            {
              if (MSC_Handle->unit[MSC_Handle->current_lun].state_changed == 1U)
              {
                USBH_UsrLog("MSC Device capacity : %u Bytes", \
                            (unsigned int)(MSC_Handle->unit[MSC_Handle->current_lun].capacity.block_nbr *
                             MSC_Handle->unit[MSC_Handle->current_lun].capacity.block_size));
                USBH_UsrLog("Block number : %u", (unsigned int)(MSC_Handle->unit[MSC_Handle->current_lun].capacity.block_nbr));
                USBH_UsrLog("Block Size   : %u", (unsigned int)(MSC_Handle->unit[MSC_Handle->current_lun].capacity.block_size));
              }
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_IDLE;
              MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_OK;
              MSC_Handle->current_lun++;
            }
            else if (scsi_status == USBH_FAIL)
            {
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_REQUEST_SENSE;
            }
            else
            {
              if ((phost->Timer - MSC_Handle->timer) >= MSC_READ_CAPACITY_TIMEOUT_MS)
              {
                USBH_UsrLog("[MSC] ReadCapacity timeout");
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_REQUEST_SENSE;
                if (phost->pUser != NULL)
                {
                  phost->pUser(phost, HOST_USER_UNRECOVERED_ERROR);
                }
              }

              if (scsi_status == USBH_UNRECOVERED_ERROR)
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_UNRECOVERED_ERROR;
                MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_ERROR;
              }
            }
            break;

          case MSC_REQUEST_SENSE:
            scsi_status = USBH_MSC_SCSI_RequestSense(phost, (uint8_t)MSC_Handle->current_lun, &MSC_Handle->unit[MSC_Handle->current_lun].sense);

            if (scsi_status == USBH_OK)
            {
              if ((MSC_Handle->unit[MSC_Handle->current_lun].sense.key == SCSI_SENSE_KEY_UNIT_ATTENTION) ||
                  (MSC_Handle->unit[MSC_Handle->current_lun].sense.key == SCSI_SENSE_KEY_NOT_READY))
              {

                if ((phost->Timer - MSC_Handle->timer) < 10000U)
                {
                  MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_TEST_UNIT_READY;
                  break;
                }
              }

              USBH_UsrLog("Sense Key  : %x", MSC_Handle->unit[MSC_Handle->current_lun].sense.key);
              USBH_UsrLog("Additional Sense Code : %x", MSC_Handle->unit[MSC_Handle->current_lun].sense.asc);
              USBH_UsrLog("Additional Sense Code Qualifier: %x", MSC_Handle->unit[MSC_Handle->current_lun].sense.ascq);
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_IDLE;
              MSC_Handle->current_lun++;
            }
            else if (scsi_status == USBH_FAIL)
            {
              USBH_UsrLog("MSC Device NOT ready");
              MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_UNRECOVERED_ERROR;
              MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_ERROR;
            }
            else
            {
              if ((phost->Timer - MSC_Handle->timer) >= MSC_REQUEST_SENSE_TIMEOUT_MS)
              {
                USBH_UsrLog("[MSC] RequestSense timeout");
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_UNRECOVERED_ERROR;
                MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_ERROR;
                if (phost->pUser != NULL)
                {
                  phost->pUser(phost, HOST_USER_UNRECOVERED_ERROR);
                }
              }

              if (scsi_status == USBH_UNRECOVERED_ERROR)
              {
                MSC_Handle->unit[MSC_Handle->current_lun].state = MSC_UNRECOVERED_ERROR;
                MSC_Handle->unit[MSC_Handle->current_lun].error = MSC_ERROR;
              }
            }
            break;

          case MSC_UNRECOVERED_ERROR:
            MSC_Handle->current_lun++;
            break;

          default:
            break;
        }

#if (USBH_USE_OS == 1U)
        USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      }
      else
      {
        MSC_Handle->current_lun = 0U;
        MSC_Handle->state = MSC_USER_NOTIFY;

#if (USBH_USE_OS == 1U)
        USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      }
      break;

    case MSC_USER_NOTIFY:
      if (MSC_Handle->lun < MSC_Handle->max_lun)
      {
        MSC_Handle->current_lun = MSC_Handle->lun;
        if (MSC_Handle->unit[MSC_Handle->current_lun].error == MSC_OK)
        {
          phost->pUser(phost, HOST_USER_CLASS_ACTIVE);
        }
        else
        {
          phost->pUser(phost, HOST_USER_UNRECOVERED_ERROR);
        }

        MSC_Handle->lun++;
      }
      else
      {
        MSC_Handle->lun = 0U;
        MSC_Handle->state = MSC_IDLE;
      }

#if (USBH_USE_OS == 1U)
      USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      break;

    case MSC_IDLE:
      error = USBH_OK;
      break;

    default:
      break;
  }
  return error;
}

static const char *USBH_MSC_StateName(uint8_t state)
{
  switch (state)
  {
    case MSC_INIT:
      return "MSC_INIT";
    case MSC_READ_INQUIRY:
      return "MSC_READ_INQUIRY";
    case MSC_TEST_UNIT_READY:
      return "MSC_TEST_UNIT_READY";
    case MSC_READ_CAPACITY10:
      return "MSC_READ_CAPACITY10";
    case MSC_REQUEST_SENSE:
      return "MSC_REQUEST_SENSE";
    case MSC_UNRECOVERED_ERROR:
      return "MSC_UNRECOVERED_ERROR";
    case MSC_USER_NOTIFY:
      return "MSC_USER_NOTIFY";
    case MSC_IDLE:
      return "MSC_IDLE";
    default:
      return "MSC_UNKNOWN";
  }
}


/**
  * @brief  USBH_MSC_SOFProcess
  *         The function is for SOF state
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MSC_SOFProcess(USBH_HandleTypeDef *phost)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(phost);

  return USBH_OK;
}
/**
  * @brief  USBH_MSC_RdWrProcess
  *         The function is for managing state machine for MSC I/O Process
  * @param  phost: Host handle
  * @param  lun: logical Unit Number
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MSC_RdWrProcess(USBH_HandleTypeDef *phost, uint8_t lun)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;
  USBH_StatusTypeDef error = USBH_BUSY;
  USBH_StatusTypeDef scsi_status = USBH_BUSY;

  /* Switch MSC REQ state machine */
  switch (MSC_Handle->unit[lun].state)
  {
    case MSC_READ:
      scsi_status = USBH_MSC_SCSI_Read(phost, lun, 0U, NULL, 0U);

      if (scsi_status == USBH_OK)
      {
        MSC_Handle->unit[lun].state = MSC_IDLE;
        error = USBH_OK;
      }
      else if (scsi_status == USBH_FAIL)
      {
        MSC_Handle->unit[lun].state = MSC_REQUEST_SENSE;
      }
      else
      {
        if (scsi_status == USBH_UNRECOVERED_ERROR)
        {
          MSC_Handle->unit[lun].state = MSC_UNRECOVERED_ERROR;
          error = USBH_FAIL;
        }
      }

#if (USBH_USE_OS == 1U)
      USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      break;

    case MSC_WRITE:
      scsi_status = USBH_MSC_SCSI_Write(phost, lun, 0U, NULL, 0U);

      if (scsi_status == USBH_OK)
      {
        MSC_Handle->unit[lun].state = MSC_IDLE;
        error = USBH_OK;
      }
      else if (scsi_status == USBH_FAIL)
      {
        MSC_Handle->unit[lun].state = MSC_REQUEST_SENSE;
      }
      else
      {
        if (scsi_status == USBH_UNRECOVERED_ERROR)
        {
          MSC_Handle->unit[lun].state = MSC_UNRECOVERED_ERROR;
          error = USBH_FAIL;
        }
      }

#if (USBH_USE_OS == 1U)
      USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      break;

    case MSC_REQUEST_SENSE:
      scsi_status = USBH_MSC_SCSI_RequestSense(phost, lun, &MSC_Handle->unit[lun].sense);

      if (scsi_status == USBH_OK)
      {
        USBH_UsrLog("Sense Key  : %x", MSC_Handle->unit[lun].sense.key);
        USBH_UsrLog("Additional Sense Code : %x", MSC_Handle->unit[lun].sense.asc);
        USBH_UsrLog("Additional Sense Code Qualifier: %x", MSC_Handle->unit[lun].sense.ascq);
        MSC_Handle->unit[lun].state = MSC_IDLE;
        MSC_Handle->unit[lun].error = MSC_ERROR;

        error = USBH_FAIL;
      }
      else if (scsi_status == USBH_FAIL)
      {
        USBH_UsrLog("MSC Device NOT ready");
      }
      else
      {
        if (scsi_status == USBH_UNRECOVERED_ERROR)
        {
          MSC_Handle->unit[lun].state = MSC_UNRECOVERED_ERROR;
          error = USBH_FAIL;
        }
      }

#if (USBH_USE_OS == 1U)
      USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      break;

    default:
      break;

  }
  return error;
}

/**
  * @brief  USBH_MSC_IsReady
  *         The function check if the MSC function is ready
  * @param  phost: Host handle
  * @retval USBH Status
  */
uint8_t USBH_MSC_IsReady(USBH_HandleTypeDef *phost)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;
  uint8_t res;

  if ((phost->gState == HOST_CLASS) && (MSC_Handle->state == MSC_IDLE))
  {
    res = 1U;
  }
  else
  {
    res = 0U;
  }

  return res;
}

/**
  * @brief  USBH_MSC_GetMaxLUN
  *         The function return the Max LUN supported
  * @param  phost: Host handle
  * @retval logical Unit Number supported
  */
uint8_t USBH_MSC_GetMaxLUN(USBH_HandleTypeDef *phost)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;

  if ((phost->gState == HOST_CLASS) && (MSC_Handle->state == MSC_IDLE))
  {
    return (uint8_t)MSC_Handle->max_lun;
  }

  return 0xFFU;
}

/**
  * @brief  USBH_MSC_UnitIsReady
  *         The function check whether a LUN is ready
  * @param  phost: Host handle
  * @param  lun: logical Unit Number
  * @retval Lun status (0: not ready / 1: ready)
  */
uint8_t USBH_MSC_UnitIsReady(USBH_HandleTypeDef *phost, uint8_t lun)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;
  uint8_t res;

  /* Store the current lun */
  MSC_Handle->current_lun = lun;

  if ((phost->gState == HOST_CLASS) && (MSC_Handle->unit[lun].error == MSC_OK))
  {
    res = 1U;
  }
  else
  {
    res = 0U;
  }

  return res;
}

/**
  * @brief  USBH_MSC_GetLUNInfo
  *         The function return a LUN information
  * @param  phost: Host handle
  * @param  lun: logical Unit Number
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_MSC_GetLUNInfo(USBH_HandleTypeDef *phost, uint8_t lun, MSC_LUNTypeDef *info)
{
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;

  /* Store the current lun */
  MSC_Handle->current_lun = lun;

  if (phost->gState == HOST_CLASS)
  {
    (void)USBH_memcpy(info, &MSC_Handle->unit[lun], sizeof(MSC_LUNTypeDef));
    return USBH_OK;
  }
  else
  {
    return USBH_FAIL;
  }
}

/**
  * @brief  USBH_MSC_Read
  *         The function performs a Read operation
  * @param  phost: Host handle
  * @param  lun: logical Unit Number
  * @param  address: sector address
  * @param  pbuf: pointer to data
  * @param  length: number of sector to read
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_MSC_Read(USBH_HandleTypeDef *phost,
                                 uint8_t lun,
                                 uint32_t address,
                                 uint8_t *pbuf,
                                 uint32_t length)
{
  uint32_t timeout;
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;

  /* Store the current lun */
  MSC_Handle->current_lun = lun;

  if ((phost->device.PortEnabled == 0U) ||
      (phost->gState != HOST_CLASS) ||
      (MSC_Handle->unit[lun].state != MSC_IDLE))
  {
    return  USBH_FAIL;
  }

  MSC_Handle->unit[lun].state = MSC_READ;

  (void)USBH_MSC_SCSI_Read(phost, lun, address, pbuf, length);

  timeout = phost->Timer;

  while (USBH_MSC_RdWrProcess(phost, lun) == USBH_BUSY)
  {
    if (((phost->Timer - timeout) > (10000U * length)) || (phost->device.PortEnabled == 0U))
    {
      HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;
      uint32_t in_pipe = MSC_Handle->InPipe;
      uint32_t out_pipe = MSC_Handle->OutPipe;
      USB_OTG_HostChannelTypeDef *in_hc_regs = NULL;
      USB_OTG_HostChannelTypeDef *out_hc_regs = NULL;

      g_msc_read_timeout_snapshot.signature = 0x4D534354U; /* MSCT */
      g_msc_read_timeout_snapshot.timer_now = phost->Timer;
      g_msc_read_timeout_snapshot.timer_start = timeout;
      g_msc_read_timeout_snapshot.address = address;
      g_msc_read_timeout_snapshot.length = length;
      g_msc_read_timeout_snapshot.host_gstate = phost->gState;
      g_msc_read_timeout_snapshot.port_enabled = phost->device.PortEnabled;
      g_msc_read_timeout_snapshot.current_lun = MSC_Handle->current_lun;
      g_msc_read_timeout_snapshot.unit_state = MSC_Handle->unit[lun].state;
      g_msc_read_timeout_snapshot.unit_error = MSC_Handle->unit[lun].error;
      g_msc_read_timeout_snapshot.bot_state = MSC_Handle->hbot.state;
      g_msc_read_timeout_snapshot.bot_cmd_state = MSC_Handle->hbot.cmd_state;
      g_msc_read_timeout_snapshot.in_pipe = in_pipe;
      g_msc_read_timeout_snapshot.out_pipe = out_pipe;

      if (hhcd != NULL)
      {
        in_hc_regs = (USB_OTG_HostChannelTypeDef *)((uint32_t)hhcd->Instance +
                                                    USB_OTG_HOST_CHANNEL_BASE +
                                                    (in_pipe * USB_OTG_HOST_CHANNEL_SIZE));
        out_hc_regs = (USB_OTG_HostChannelTypeDef *)((uint32_t)hhcd->Instance +
                                                     USB_OTG_HOST_CHANNEL_BASE +
                                                     (out_pipe * USB_OTG_HOST_CHANNEL_SIZE));
        g_msc_read_timeout_snapshot.hhcd_state = hhcd->State;
        g_msc_read_timeout_snapshot.hhcd_error_code = hhcd->ErrorCode;
        g_msc_read_timeout_snapshot.in_urb_state = hhcd->hc[in_pipe].urb_state;
        g_msc_read_timeout_snapshot.out_urb_state = hhcd->hc[out_pipe].urb_state;
        g_msc_read_timeout_snapshot.in_hc_state = hhcd->hc[in_pipe].state;
        g_msc_read_timeout_snapshot.out_hc_state = hhcd->hc[out_pipe].state;
        g_msc_read_timeout_snapshot.in_err_cnt = hhcd->hc[in_pipe].ErrCnt;
        g_msc_read_timeout_snapshot.out_err_cnt = hhcd->hc[out_pipe].ErrCnt;
        g_msc_read_timeout_snapshot.in_hcchar = in_hc_regs->HCCHAR;
        g_msc_read_timeout_snapshot.in_hcint = in_hc_regs->HCINT;
        g_msc_read_timeout_snapshot.in_hcintmsk = in_hc_regs->HCINTMSK;
        g_msc_read_timeout_snapshot.in_hctsiz = in_hc_regs->HCTSIZ;
        g_msc_read_timeout_snapshot.out_hcchar = out_hc_regs->HCCHAR;
        g_msc_read_timeout_snapshot.out_hcint = out_hc_regs->HCINT;
        g_msc_read_timeout_snapshot.out_hcintmsk = out_hc_regs->HCINTMSK;
        g_msc_read_timeout_snapshot.out_hctsiz = out_hc_regs->HCTSIZ;
      }
      else
      {
        g_msc_read_timeout_snapshot.hhcd_state = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.hhcd_error_code = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_urb_state = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_urb_state = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_hc_state = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_hc_state = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_err_cnt = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_err_cnt = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_hcchar = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_hcint = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_hcintmsk = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.in_hctsiz = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_hcchar = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_hcint = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_hcintmsk = 0xFFFFFFFFU;
        g_msc_read_timeout_snapshot.out_hctsiz = 0xFFFFFFFFU;
      }

      printf("[MSC-READ] timeout/fail: lun=%u addr=%lu len=%lu timer=%lu start=%lu port=%u unit_state=%u unit_err=%u bot_state=%u bot_cmd=%u\n",
             (unsigned)lun,
             (unsigned long)address,
             (unsigned long)length,
             (unsigned long)phost->Timer,
             (unsigned long)timeout,
             (unsigned)phost->device.PortEnabled,
             (unsigned)MSC_Handle->unit[lun].state,
             (unsigned)MSC_Handle->unit[lun].error,
             (unsigned)MSC_Handle->hbot.state,
             (unsigned)MSC_Handle->hbot.cmd_state);
      return USBH_FAIL;
    }
  }

  return USBH_OK;
}

/**
  * @brief  USBH_MSC_Write
  *         The function performs a Write operation
  * @param  phost: Host handle
  * @param  lun: logical Unit Number
  * @param  address: sector address
  * @param  pbuf: pointer to data
  * @param  length: number of sector to write
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_MSC_Write(USBH_HandleTypeDef *phost,
                                  uint8_t lun,
                                  uint32_t address,
                                  uint8_t *pbuf,
                                  uint32_t length)
{
  uint32_t timeout;
  MSC_HandleTypeDef *MSC_Handle = (MSC_HandleTypeDef *) phost->pActiveClass->pData;

  /* Store the current lun */
  MSC_Handle->current_lun = lun;

  if ((phost->device.PortEnabled == 0U) ||
      (phost->gState != HOST_CLASS) ||
      (MSC_Handle->unit[lun].state != MSC_IDLE))
  {
    return  USBH_FAIL;
  }

  MSC_Handle->unit[lun].state = MSC_WRITE;

  (void)USBH_MSC_SCSI_Write(phost, lun, address, pbuf, length);

  timeout = phost->Timer;
  while (USBH_MSC_RdWrProcess(phost, lun) == USBH_BUSY)
  {
    if (((phost->Timer - timeout) > (10000U * length)) || (phost->device.PortEnabled == 0U))
    {
      return USBH_FAIL;
    }
  }

  return USBH_OK;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
