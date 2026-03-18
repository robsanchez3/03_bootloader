################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.c 

OBJS += \
./Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.o 

C_DEPS += \
./Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.o: C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.c Drivers/Vendor/FatFs/source/drivers/USB/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../FATFS/App -I../../FATFS/Target -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/FatFs/source/drivers/USB -I../../USB_Host/Target -I../../USB_Host/App -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Vendor-2f-FatFs-2f-source-2f-drivers-2f-USB

clean-Drivers-2f-Vendor-2f-FatFs-2f-source-2f-drivers-2f-USB:
	-$(RM) ./Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.cyclo ./Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.d ./Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.o ./Drivers/Vendor/FatFs/source/drivers/USB/usbh_diskio.su

.PHONY: clean-Drivers-2f-Vendor-2f-FatFs-2f-source-2f-drivers-2f-USB

