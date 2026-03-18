/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : USB_Host/Target/usbh_conf.c
  * @brief          : USB Host low level hardware driver - STM32U5 adaptation
  *
  * Adapted from OztDUI_UsbFlash (STM32F7) reference project.
  * Key changes vs reference:
  *   - USB_OTG_FS  -> USB_OTG_HS (embedded PHY, High Speed)
  *   - HOST_FS     -> HOST_HS
  *   - HCD_SPEED_FULL -> HCD_SPEED_HIGH
  *   - GPIO_AF10_OTG_FS -> GPIO_AF10_USB_HS
  *   - OTG_FS_IRQn -> OTG_HS_IRQn  (priority 5, FreeRTOS compatible)
  *   - RCC: USBPHYC clock + PLL1/DIV2
  *   - PWR: VddUSB + HS transceiver supply
  *   - SYSCFG: OTG PHY enable + reference clock selection
  *   - VBUS: controlled via PI15 (FS_PW_SW) power switch
  *   - 16 host channels, DMA enabled
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include <stdio.h>

/* USER CODE BEGIN Includes */
#define USB_HOST_DEBUG
#ifdef USB_HOST_DEBUG
#define USB_LOG(...)  printf(__VA_ARGS__)
#else
#define USB_LOG(...)  do {} while(0)
#endif
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
HCD_HandleTypeDef  hhcd_USB_OTG_HS;
void Error_Handler(void);

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status);

/*******************************************************************************
                    LL Driver Callbacks (HCD -> USB Host Library)
*******************************************************************************/

/**
  * @brief  HCD MSP Init - STM32U5 USB_OTG_HS hardware initialisation
  */
void HAL_HCD_MspInit(HCD_HandleTypeDef *hcdHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (hcdHandle->Instance == USB_OTG_HS)
  {
    /* USER CODE BEGIN USB_OTG_HS_MspInit 0 */
    USB_LOG("[USB] HAL_HCD_MspInit start\n");
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    /* USER CODE END USB_OTG_HS_MspInit 0 */

    /* USB PHY clock: PLL1 / 2 = 16 MHz */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
    PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_PLL1_DIV2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* OTG PHY reference clock selection */
    HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

    /* GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();   /* PI15 = FS_PW_SW, PI14 = FS_OV_Current */

    /**USB_OTG_HS GPIO Configuration
       PA9  ------> USB_OTG_HS_VBUS  (input, VBUS sense)
       PA10 ------> USB_OTG_HS_ID    (alternate function, OTG ID)
       PA11 ------> USB_OTG_HS_DM
       PA12 ------> USB_OTG_HS_DP
    */
    /* PA10: ID pin */
    GPIO_InitStruct.Pin       = GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_USB_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA9: VBUS sense (input only) */
    GPIO_InitStruct.Pin  = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA11, PA12: D-, D+ */
    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_USB_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PI15: FS_PW_SW - VBUS power switch, active-low: init HIGH = VBUS off */
    GPIO_InitStruct.Pin   = FS_PW_SW_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(FS_PW_SW_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);

    /* Peripheral clocks */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USBPHYC_CLK_ENABLE();

    /* Enable VDDUSB and HS transceiver */
    if (__HAL_RCC_PWR_IS_CLK_DISABLED())
    {
      __HAL_RCC_PWR_CLK_ENABLE();
      HAL_PWREx_EnableVddUSB();
      HAL_PWREx_EnableUSBHSTranceiverSupply();
      __HAL_RCC_PWR_CLK_DISABLE();
    }
    else
    {
      HAL_PWREx_EnableVddUSB();
      HAL_PWREx_EnableUSBHSTranceiverSupply();
    }

    /* Enable OTG HS PHY */
    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

    /* USB_OTG_HS interrupt - priority 5 (FreeRTOS compatible) */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);

    /* USER CODE BEGIN USB_OTG_HS_MspInit 1 */
    USB_LOG("[USB] HAL_HCD_MspInit complete\n");
    /* USER CODE END USB_OTG_HS_MspInit 1 */
  }
}

/**
  * @brief  HCD MSP DeInit
  */
void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hcdHandle)
{
  if (hcdHandle->Instance == USB_OTG_HS)
  {
    /* USER CODE BEGIN USB_OTG_HS_MspDeInit 0 */

    /* USER CODE END USB_OTG_HS_MspDeInit 0 */

    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USBPHYC_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_RESET);

    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);

    /* USER CODE BEGIN USB_OTG_HS_MspDeInit 1 */

    /* USER CODE END USB_OTG_HS_MspDeInit 1 */
  }
}

/* HCD -> USBH Library callbacks ------------------------------------------- */

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_LL_IncTimer(hhcd->pData);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_LL_Connect(hhcd->pData);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_LL_Disconnect(hhcd->pData);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
                                          HCD_URBStateTypeDef urb_state)
{
#if (USBH_USE_OS == 1U)
  USBH_LL_NotifyURBChange(hhcd->pData);
#endif
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_LL_PortEnabled(hhcd->pData);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_LL_PortDisabled(hhcd->pData);
}

/*******************************************************************************
                  LL Driver Interface (USB Host Library --> HCD)
*******************************************************************************/

/**
  * @brief  Initialize the low level portion of the host driver (HCD init)
  */
USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
  if (phost->id == HOST_HS)
  {
    hhcd_USB_OTG_HS.pData  = phost;
    phost->pData           = &hhcd_USB_OTG_HS;

    hhcd_USB_OTG_HS.Instance           = USB_OTG_HS;
    hhcd_USB_OTG_HS.Init.Host_channels = 16;
    hhcd_USB_OTG_HS.Init.speed         = HCD_SPEED_HIGH;
    hhcd_USB_OTG_HS.Init.dma_enable    = DISABLE;
    hhcd_USB_OTG_HS.Init.phy_itface    = USB_OTG_HS_EMBEDDED_PHY;
    hhcd_USB_OTG_HS.Init.Sof_enable    = DISABLE;

    if (HAL_HCD_Init(&hhcd_USB_OTG_HS) != HAL_OK)
    {
      Error_Handler();
    }

    USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd_USB_OTG_HS));
  }
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
  return USBH_Get_USB_Status(HAL_HCD_DeInit(phost->pData));
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
  return USBH_Get_USB_Status(HAL_HCD_Start(phost->pData));
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
  return USBH_Get_USB_Status(HAL_HCD_Stop(phost->pData));
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
  switch (HAL_HCD_GetCurrentSpeed(phost->pData))
  {
    case 0:  return USBH_SPEED_HIGH;
    case 1:  return USBH_SPEED_FULL;
    case 2:  return USBH_SPEED_LOW;
    default: return USBH_SPEED_FULL;
  }
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
  return USBH_Get_USB_Status(HAL_HCD_ResetPort(phost->pData));
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe_num,
                                     uint8_t epnum, uint8_t dev_address, uint8_t speed,
                                     uint8_t ep_type, uint16_t mps)
{
  return USBH_Get_USB_Status(
    HAL_HCD_HC_Init(phost->pData, pipe_num, epnum, dev_address, speed, ep_type, mps));
}

USBH_StatusTypeDef USBH_LL_ActivatePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return USBH_Get_USB_Status(HAL_HCD_HC_Activate(phost->pData, pipe));
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return USBH_Get_USB_Status(HAL_HCD_HC_Halt(phost->pData, pipe));
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe,
                                      uint8_t direction, uint8_t ep_type, uint8_t token,
                                      uint8_t *pbuff, uint16_t length, uint8_t do_ping)
{
  return USBH_Get_USB_Status(
    HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction, ep_type, token,
                             pbuff, length, do_ping));
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(phost->pData, pipe);
}

/**
  * @brief  VBUS control via PI15 (FS_PW_SW) power switch
  *         state=1 -> enable VBUS (power pen drive)
  *         state=0 -> disable VBUS
  */
USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */

  if (phost->id == HOST_HS)
  {
    if (state == 0U)
    {
      USB_LOG("[USB] VBUS OFF (PI15=SET, active-low switch)\n");
      HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);
    }
    else
    {
      USB_LOG("[USB] VBUS ON (PI15=RESET, active-low switch)\n");
      HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_RESET);
    }
  }

  HAL_Delay(200);
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
  HCD_HandleTypeDef *pHandle = phost->pData;

  if (pHandle->hc[pipe].ep_is_in)
    pHandle->hc[pipe].toggle_in  = toggle;
  else
    pHandle->hc[pipe].toggle_out = toggle;

  return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  HCD_HandleTypeDef *pHandle = phost->pData;

  if (pHandle->hc[pipe].ep_is_in)
    return pHandle->hc[pipe].toggle_in;
  else
    return pHandle->hc[pipe].toggle_out;
}

void USBH_Delay(uint32_t Delay)
{
	HAL_Delay(Delay);
}

/**
  * @brief  Map HAL status to USBH status
  */
USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
  switch (hal_status)
  {
    case HAL_OK:      return USBH_OK;
    case HAL_BUSY:    return USBH_BUSY;
    case HAL_ERROR:
    case HAL_TIMEOUT:
    default:          return USBH_FAIL;
  }
}
