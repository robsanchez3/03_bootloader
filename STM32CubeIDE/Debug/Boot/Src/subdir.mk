################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_jump.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/usb_fs_service.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/usb_msc_service.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/usb_update.c 

OBJS += \
./Boot/Src/boot_jump.o \
./Boot/Src/usb_fs_service.o \
./Boot/Src/usb_msc_service.o \
./Boot/Src/usb_update.o 

C_DEPS += \
./Boot/Src/boot_jump.d \
./Boot/Src/usb_fs_service.d \
./Boot/Src/usb_msc_service.d \
./Boot/Src/usb_update.d 


# Each subdirectory must supply rules for building sources it contributes
Boot/Src/boot_jump.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_jump.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/usb_fs_service.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/usb_fs_service.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/usb_msc_service.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/usb_msc_service.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/usb_update.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/usb_update.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boot-2f-Src

clean-Boot-2f-Src:
	-$(RM) ./Boot/Src/boot_jump.cyclo ./Boot/Src/boot_jump.d ./Boot/Src/boot_jump.o ./Boot/Src/boot_jump.su ./Boot/Src/usb_fs_service.cyclo ./Boot/Src/usb_fs_service.d ./Boot/Src/usb_fs_service.o ./Boot/Src/usb_fs_service.su ./Boot/Src/usb_msc_service.cyclo ./Boot/Src/usb_msc_service.d ./Boot/Src/usb_msc_service.o ./Boot/Src/usb_msc_service.su ./Boot/Src/usb_update.cyclo ./Boot/Src/usb_update.d ./Boot/Src/usb_update.o ./Boot/Src/usb_update.su

.PHONY: clean-Boot-2f-Src

