################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Boot/Src/boot_jump.c 

OBJS += \
./Boot/Src/boot_jump.o 

C_DEPS += \
./Boot/Src/boot_jump.d 


# Each subdirectory must supply rules for building sources it contributes
Boot/Src/%.o Boot/Src/%.su Boot/Src/%.cyclo: ../Boot/Src/%.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DUSE_HAL_DRIVER -DSTM32U599xx -c -I../Inc -I../Core/Inc -I../Boot/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Include -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boot-2f-Src

clean-Boot-2f-Src:
	-$(RM) ./Boot/Src/boot_jump.cyclo ./Boot/Src/boot_jump.d ./Boot/Src/boot_jump.o ./Boot/Src/boot_jump.su

.PHONY: clean-Boot-2f-Src

