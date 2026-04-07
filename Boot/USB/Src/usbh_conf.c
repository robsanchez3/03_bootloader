#include "usbh_core.h"
#include "usbh_msc.h"
#include "usb_msc_service.h"
#include <stdio.h>

HCD_HandleTypeDef hhcd_USB_OTG_HS;

static USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status);
#if (BOOT_USB_ST_TRACE_VERBOSE != 0U)
static const char *USBH_LL_UrbStateName(HCD_URBStateTypeDef state);
#endif
static uint32_t usb_vbus_delay_ms = 750U;

void USBH_LL_SetVbusDelay(uint32_t delay_ms)
{
    usb_vbus_delay_ms = delay_ms;
}

void USBH_LL_HardResetHostController(void)
{
    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);

    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);
    HAL_Delay(50U);

    __HAL_RCC_USB_OTG_HS_FORCE_RESET();
    HAL_Delay(10U);
    __HAL_RCC_USB_OTG_HS_RELEASE_RESET();

    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USBPHYC_CLK_DISABLE();
    HAL_Delay(10U);
    __HAL_RCC_USBPHYC_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();

    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);
    HAL_Delay(10U);

    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
}

void HAL_HCD_MspInit(HCD_HandleTypeDef *hcdHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (hcdHandle->Instance != USB_OTG_HS)
    {
        return;
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
    PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_PLL1_DIV2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_USB_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_USB_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = FS_PW_SW_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(FS_PW_SW_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);

    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USBPHYC_CLK_ENABLE();

    {
        uint8_t pwr_was_off = __HAL_RCC_PWR_IS_CLK_DISABLED();
        if (pwr_was_off)
        {
            __HAL_RCC_PWR_CLK_ENABLE();
        }
        HAL_PWREx_EnableVddUSB();
        HAL_PWREx_EnableUSBHSTranceiverSupply();
        if (pwr_was_off)
        {
            __HAL_RCC_PWR_CLK_DISABLE();
        }
    }

    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hcdHandle)
{
    if (hcdHandle->Instance != USB_OTG_HS)
    {
        return;
    }

    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USBPHYC_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);
    HAL_GPIO_DeInit(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin);

    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
    if ((hhcd != NULL) && (hhcd->pData == &hUsbHostHS))
    {
        USBH_LL_IncTimer(&hUsbHostHS);
    }
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
    if ((hhcd != NULL) && (hhcd->pData == &hUsbHostHS))
    {
        USBH_LL_Connect(&hUsbHostHS);
    }
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
    if ((hhcd != NULL) && (hhcd->pData == &hUsbHostHS))
    {
        USBH_LL_Disconnect(&hUsbHostHS);
    }
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd,
                                         uint8_t chnum,
                                         HCD_URBStateTypeDef urb_state)
{
    (void)hhcd;
    (void)chnum;
    (void)urb_state;
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
    if ((hhcd != NULL) && (hhcd->pData == &hUsbHostHS))
    {
        USBH_LL_PortEnabled(&hUsbHostHS);
    }
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
    if ((hhcd != NULL) && (hhcd->pData == &hUsbHostHS))
    {
        USBH_LL_PortDisabled(&hUsbHostHS);
    }
}

USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
    if (phost->id != HOST_HS)
    {
        return USBH_FAIL;
    }

    hhcd_USB_OTG_HS.pData = phost;
    phost->pData = &hhcd_USB_OTG_HS;

    hhcd_USB_OTG_HS.Instance = USB_OTG_HS;
    hhcd_USB_OTG_HS.Init.Host_channels = 16;
    hhcd_USB_OTG_HS.Init.speed = HCD_SPEED_HIGH;
    hhcd_USB_OTG_HS.Init.dma_enable = DISABLE;
    hhcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
    hhcd_USB_OTG_HS.Init.Sof_enable = ENABLE;

    if (HAL_HCD_Init(&hhcd_USB_OTG_HS) != HAL_OK)
    {
        return USBH_FAIL;
    }

    USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd_USB_OTG_HS));
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_DeInit((HCD_HandleTypeDef *)phost->pData));
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_Start((HCD_HandleTypeDef *)phost->pData));
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_Stop((HCD_HandleTypeDef *)phost->pData));
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
    switch (HAL_HCD_GetCurrentSpeed((HCD_HandleTypeDef *)phost->pData))
    {
        case 0U:
            return USBH_SPEED_HIGH;
        case 1U:
            return USBH_SPEED_FULL;
        case 2U:
            return USBH_SPEED_LOW;
        default:
            return USBH_SPEED_FULL;
    }
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_ResetPort((HCD_HandleTypeDef *)phost->pData));
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    return HAL_HCD_HC_GetXferCount((HCD_HandleTypeDef *)phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost,
                                    uint8_t pipe_num,
                                    uint8_t epnum,
                                    uint8_t dev_address,
                                    uint8_t speed,
                                    uint8_t ep_type,
                                    uint16_t mps)
{
    phost->Pipes[pipe_num] = epnum;
    return USBH_Get_USB_Status(HAL_HCD_HC_Init((HCD_HandleTypeDef *)phost->pData,
                                               pipe_num,
                                               epnum,
                                               dev_address,
                                               speed,
                                               ep_type,
                                               mps));
}

USBH_StatusTypeDef USBH_LL_ActivatePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    return USBH_Get_USB_Status(HAL_HCD_HC_Activate((HCD_HandleTypeDef *)phost->pData, pipe));
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    phost->Pipes[pipe] = 0U;
    return USBH_Get_USB_Status(HAL_HCD_HC_Halt((HCD_HandleTypeDef *)phost->pData, pipe));
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost,
                                     uint8_t pipe,
                                     uint8_t direction,
                                     uint8_t ep_type,
                                     uint8_t token,
                                     uint8_t *pbuff,
                                     uint16_t length,
                                     uint8_t do_ping)
{
    return USBH_Get_USB_Status(HAL_HCD_HC_SubmitRequest((HCD_HandleTypeDef *)phost->pData,
                                                        pipe,
                                                        direction,
                                                        ep_type,
                                                        token,
                                                        pbuff,
                                                        length,
                                                        do_ping));
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    HCD_URBStateTypeDef urb_state;

    urb_state = HAL_HCD_HC_GetURBState((HCD_HandleTypeDef *)phost->pData, pipe);

    if ((pipe == phost->Control.pipe_out) || (pipe == phost->Control.pipe_in))
    {
        static HCD_URBStateTypeDef last_ctrl_out = (HCD_URBStateTypeDef)0xFFU;
        static HCD_URBStateTypeDef last_ctrl_in  = (HCD_URBStateTypeDef)0xFFU;
        HCD_URBStateTypeDef *last_state = (pipe == phost->Control.pipe_out) ? &last_ctrl_out : &last_ctrl_in;

        if (urb_state != *last_state)
        {
            *last_state = urb_state;
                BOOT_USB_ST_TRACE("[USBH_LL] CTRL pipe %s urb=%s (%u)",
                                  (pipe == phost->Control.pipe_out) ? "OUT" : "IN",
                                  USBH_LL_UrbStateName(urb_state),
                                  (unsigned int)urb_state);
        }
    }
    else if ((phost->pActiveClass != NULL) && (phost->pActiveClass->pData != NULL))
    {
        MSC_HandleTypeDef *msc_handle = (MSC_HandleTypeDef *)phost->pActiveClass->pData;
        static HCD_URBStateTypeDef last_bulk_out = (HCD_URBStateTypeDef)0xFFU;
        static HCD_URBStateTypeDef last_bulk_in  = (HCD_URBStateTypeDef)0xFFU;

        if ((pipe == msc_handle->OutPipe) || (pipe == msc_handle->InPipe))
        {
            HCD_URBStateTypeDef *last_state = (pipe == msc_handle->OutPipe) ? &last_bulk_out : &last_bulk_in;

            if (urb_state != *last_state)
            {
                *last_state = urb_state;
                BOOT_USB_ST_TRACE("[USBH_LL] BULK pipe %s urb=%s (%u)",
                                  (pipe == msc_handle->OutPipe) ? "OUT" : "IN",
                                  USBH_LL_UrbStateName(urb_state),
                                  (unsigned int)urb_state);
            }
        }
    }

    return (USBH_URBStateTypeDef)urb_state;
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
    (void)phost;

    if (state == 0U)
    {
        HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(FS_PW_SW_GPIO_Port, FS_PW_SW_Pin, GPIO_PIN_RESET);
    }

    HAL_Delay(usb_vbus_delay_ms);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
    HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;

    if ((phost->Pipes[pipe] & 0x80U) != 0U)
    {
        hhcd->hc[pipe].toggle_in = toggle;
    }
    else
    {
        hhcd->hc[pipe].toggle_out = toggle;
    }

    return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;

    if ((phost->Pipes[pipe] & 0x80U) != 0U)
    {
        return hhcd->hc[pipe].toggle_in;
    }

    return hhcd->hc[pipe].toggle_out;
}

void USBH_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

static USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
    switch (hal_status)
    {
        case HAL_OK:
            return USBH_OK;
        case HAL_BUSY:
            return USBH_BUSY;
        case HAL_TIMEOUT:
            return USBH_FAIL;
        default:
            return USBH_FAIL;
    }
}

#if (BOOT_USB_ST_TRACE_VERBOSE != 0U)
static const char *USBH_LL_UrbStateName(HCD_URBStateTypeDef state)
{
    switch (state)
    {
        case URB_IDLE:
            return "IDLE";
        case URB_DONE:
            return "DONE";
        case URB_NOTREADY:
            return "NOTREADY";
        case URB_NYET:
            return "NYET";
        case URB_ERROR:
            return "ERROR";
        case URB_STALL:
            return "STALL";
        default:
            return "UNKNOWN";
    }
}
#endif
