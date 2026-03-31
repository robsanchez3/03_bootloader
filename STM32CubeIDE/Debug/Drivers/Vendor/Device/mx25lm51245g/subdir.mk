################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.c 

OBJS += \
./Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.o 

C_DEPS += \
./Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.o: C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.c Drivers/Vendor/Device/mx25lm51245g/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Vendor-2f-Device-2f-mx25lm51245g

clean-Drivers-2f-Vendor-2f-Device-2f-mx25lm51245g:
	-$(RM) ./Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.cyclo ./Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.d ./Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.o ./Drivers/Vendor/Device/mx25lm51245g/mx25lm51245g.su

.PHONY: clean-Drivers-2f-Vendor-2f-Device-2f-mx25lm51245g

