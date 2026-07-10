################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_crc.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_crypto.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_display.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_flash.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_jump.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_manifest.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_ospi.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/sw_aes.c \
C:/Roberto/O3/O3_Bootloader/Boot/Src/sw_sha256.c 

OBJS += \
./Boot/Src/boot_crc.o \
./Boot/Src/boot_crypto.o \
./Boot/Src/boot_display.o \
./Boot/Src/boot_flash.o \
./Boot/Src/boot_jump.o \
./Boot/Src/boot_manifest.o \
./Boot/Src/boot_ospi.o \
./Boot/Src/sw_aes.o \
./Boot/Src/sw_sha256.o 

C_DEPS += \
./Boot/Src/boot_crc.d \
./Boot/Src/boot_crypto.d \
./Boot/Src/boot_display.d \
./Boot/Src/boot_flash.d \
./Boot/Src/boot_jump.d \
./Boot/Src/boot_manifest.d \
./Boot/Src/boot_ospi.d \
./Boot/Src/sw_aes.d \
./Boot/Src/sw_sha256.d 


# Each subdirectory must supply rules for building sources it contributes
Boot/Src/boot_crc.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_crc.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_crypto.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_crypto.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_display.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_display.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_flash.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_flash.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_jump.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_jump.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_manifest.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_manifest.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/boot_ospi.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/boot_ospi.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/sw_aes.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/sw_aes.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Boot/Src/sw_sha256.o: C:/Roberto/O3/O3_Bootloader/Boot/Src/sw_sha256.c Boot/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/Device/mx25lm51245g -I../../FATFS/Target -I../../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boot-2f-Src

clean-Boot-2f-Src:
	-$(RM) ./Boot/Src/boot_crc.cyclo ./Boot/Src/boot_crc.d ./Boot/Src/boot_crc.o ./Boot/Src/boot_crc.su ./Boot/Src/boot_crypto.cyclo ./Boot/Src/boot_crypto.d ./Boot/Src/boot_crypto.o ./Boot/Src/boot_crypto.su ./Boot/Src/boot_display.cyclo ./Boot/Src/boot_display.d ./Boot/Src/boot_display.o ./Boot/Src/boot_display.su ./Boot/Src/boot_flash.cyclo ./Boot/Src/boot_flash.d ./Boot/Src/boot_flash.o ./Boot/Src/boot_flash.su ./Boot/Src/boot_jump.cyclo ./Boot/Src/boot_jump.d ./Boot/Src/boot_jump.o ./Boot/Src/boot_jump.su ./Boot/Src/boot_manifest.cyclo ./Boot/Src/boot_manifest.d ./Boot/Src/boot_manifest.o ./Boot/Src/boot_manifest.su ./Boot/Src/boot_ospi.cyclo ./Boot/Src/boot_ospi.d ./Boot/Src/boot_ospi.o ./Boot/Src/boot_ospi.su ./Boot/Src/sw_aes.cyclo ./Boot/Src/sw_aes.d ./Boot/Src/sw_aes.o ./Boot/Src/sw_aes.su ./Boot/Src/sw_sha256.cyclo ./Boot/Src/sw_sha256.d ./Boot/Src/sw_sha256.o ./Boot/Src/sw_sha256.su

.PHONY: clean-Boot-2f-Src

