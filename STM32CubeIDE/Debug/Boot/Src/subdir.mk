################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_flash.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_jump.c 

OBJS += \
./Boot/Src/boot_flash.o \
./Boot/Src/boot_jump.o 

C_DEPS += \
./Boot/Src/boot_flash.d \
./Boot/Src/boot_jump.d 


# Each subdirectory must supply rules for building sources it contributes
Boot/Src/boot_flash.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_flash.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../FATFS/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_jump.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_jump.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../FATFS/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boot-2f-Src

clean-Boot-2f-Src:
	-$(RM) ./Boot/Src/boot_flash.cyclo ./Boot/Src/boot_flash.d ./Boot/Src/boot_flash.o ./Boot/Src/boot_flash.su ./Boot/Src/boot_jump.cyclo ./Boot/Src/boot_jump.d ./Boot/Src/boot_jump.o ./Boot/Src/boot_jump.su

.PHONY: clean-Boot-2f-Src

