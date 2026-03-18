################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/USB_Host/Target/usbh_conf.c 

OBJS += \
./USB_Host/Target/usbh_conf.o 

C_DEPS += \
./USB_Host/Target/usbh_conf.d 


# Each subdirectory must supply rules for building sources it contributes
USB_Host/Target/usbh_conf.o: C:/Roberto/O3/O3_Bootloader/USB_Host/Target/usbh_conf.c USB_Host/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../FATFS/App -I../../FATFS/Target -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/FatFs/source/drivers/USB -I../../USB_Host/Target -I../../USB_Host/App -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-USB_Host-2f-Target

clean-USB_Host-2f-Target:
	-$(RM) ./USB_Host/Target/usbh_conf.cyclo ./USB_Host/Target/usbh_conf.d ./USB_Host/Target/usbh_conf.o ./USB_Host/Target/usbh_conf.su

.PHONY: clean-USB_Host-2f-Target

