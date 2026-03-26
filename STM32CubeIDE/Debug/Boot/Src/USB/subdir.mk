################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usb_fs_service.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usb_msc_service.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usb_update.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usbh_conf.c 

OBJS += \
./Boot/Src/USB/usb_fs_service.o \
./Boot/Src/USB/usb_msc_service.o \
./Boot/Src/USB/usb_update.o \
./Boot/Src/USB/usbh_conf.o 

C_DEPS += \
./Boot/Src/USB/usb_fs_service.d \
./Boot/Src/USB/usb_msc_service.d \
./Boot/Src/USB/usb_update.d \
./Boot/Src/USB/usbh_conf.d 


# Each subdirectory must supply rules for building sources it contributes
Boot/Src/USB/usb_fs_service.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usb_fs_service.c Boot/Src/USB/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Boot/Inc/USB -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/USB/usb_msc_service.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usb_msc_service.c Boot/Src/USB/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Boot/Inc/USB -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/USB/usb_update.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usb_update.c Boot/Src/USB/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Boot/Inc/USB -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/USB/usbh_conf.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/USB/usbh_conf.c Boot/Src/USB/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Boot/Inc/USB -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boot-2f-Src-2f-USB

clean-Boot-2f-Src-2f-USB:
	-$(RM) ./Boot/Src/USB/usb_fs_service.cyclo ./Boot/Src/USB/usb_fs_service.d ./Boot/Src/USB/usb_fs_service.o ./Boot/Src/USB/usb_fs_service.su ./Boot/Src/USB/usb_msc_service.cyclo ./Boot/Src/USB/usb_msc_service.d ./Boot/Src/USB/usb_msc_service.o ./Boot/Src/USB/usb_msc_service.su ./Boot/Src/USB/usb_update.cyclo ./Boot/Src/USB/usb_update.d ./Boot/Src/USB/usb_update.o ./Boot/Src/USB/usb_update.su ./Boot/Src/USB/usbh_conf.cyclo ./Boot/Src/USB/usbh_conf.d ./Boot/Src/USB/usbh_conf.o ./Boot/Src/USB/usbh_conf.su

.PHONY: clean-Boot-2f-Src-2f-USB

