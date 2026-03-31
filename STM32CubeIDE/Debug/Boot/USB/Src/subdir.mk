################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usb_fs_service.c \
C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usb_msc_service.c \
C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usb_update.c \
C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usbh_conf.c \
C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usbh_diskio.c 

OBJS += \
./Boot/USB/Src/usb_fs_service.o \
./Boot/USB/Src/usb_msc_service.o \
./Boot/USB/Src/usb_update.o \
./Boot/USB/Src/usbh_conf.o \
./Boot/USB/Src/usbh_diskio.o 

C_DEPS += \
./Boot/USB/Src/usb_fs_service.d \
./Boot/USB/Src/usb_msc_service.d \
./Boot/USB/Src/usb_update.d \
./Boot/USB/Src/usbh_conf.d \
./Boot/USB/Src/usbh_diskio.d 


# Each subdirectory must supply rules for building sources it contributes
Boot/USB/Src/usb_fs_service.o: C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usb_fs_service.c Boot/USB/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/USB/Src/usb_msc_service.o: C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usb_msc_service.c Boot/USB/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/USB/Src/usb_update.o: C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usb_update.c Boot/USB/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/USB/Src/usbh_conf.o: C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usbh_conf.c Boot/USB/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/USB/Src/usbh_diskio.o: C:/Roberto/O3/O3_Bootloader/Boot/USB/Src/usbh_diskio.c Boot/USB/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boot-2f-USB-2f-Src

clean-Boot-2f-USB-2f-Src:
	-$(RM) ./Boot/USB/Src/usb_fs_service.cyclo ./Boot/USB/Src/usb_fs_service.d ./Boot/USB/Src/usb_fs_service.o ./Boot/USB/Src/usb_fs_service.su ./Boot/USB/Src/usb_msc_service.cyclo ./Boot/USB/Src/usb_msc_service.d ./Boot/USB/Src/usb_msc_service.o ./Boot/USB/Src/usb_msc_service.su ./Boot/USB/Src/usb_update.cyclo ./Boot/USB/Src/usb_update.d ./Boot/USB/Src/usb_update.o ./Boot/USB/Src/usb_update.su ./Boot/USB/Src/usbh_conf.cyclo ./Boot/USB/Src/usbh_conf.d ./Boot/USB/Src/usbh_conf.o ./Boot/USB/Src/usbh_conf.su ./Boot/USB/Src/usbh_diskio.cyclo ./Boot/USB/Src/usbh_diskio.d ./Boot/USB/Src/usbh_diskio.o ./Boot/USB/Src/usbh_diskio.su

.PHONY: clean-Boot-2f-USB-2f-Src

