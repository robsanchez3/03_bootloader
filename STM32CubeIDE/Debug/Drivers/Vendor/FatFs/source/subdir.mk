################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/diskio.c \
C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/ff.c \
C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/ff_gen_drv.c \
C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/ffsystem_baremetal.c 

OBJS += \
./Drivers/Vendor/FatFs/source/diskio.o \
./Drivers/Vendor/FatFs/source/ff.o \
./Drivers/Vendor/FatFs/source/ff_gen_drv.o \
./Drivers/Vendor/FatFs/source/ffsystem_baremetal.o 

C_DEPS += \
./Drivers/Vendor/FatFs/source/diskio.d \
./Drivers/Vendor/FatFs/source/ff.d \
./Drivers/Vendor/FatFs/source/ff_gen_drv.d \
./Drivers/Vendor/FatFs/source/ffsystem_baremetal.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Vendor/FatFs/source/diskio.o: C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/diskio.c Drivers/Vendor/FatFs/source/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../FATFS/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/Vendor/FatFs/source/ff.o: C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/ff.c Drivers/Vendor/FatFs/source/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../FATFS/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/Vendor/FatFs/source/ff_gen_drv.o: C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/ff_gen_drv.c Drivers/Vendor/FatFs/source/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../FATFS/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/Vendor/FatFs/source/ffsystem_baremetal.o: C:/Roberto/O3/O3_Bootloader/Drivers/Vendor/FatFs/source/ffsystem_baremetal.c Drivers/Vendor/FatFs/source/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Boot/USB/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../FATFS/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Vendor-2f-FatFs-2f-source

clean-Drivers-2f-Vendor-2f-FatFs-2f-source:
	-$(RM) ./Drivers/Vendor/FatFs/source/diskio.cyclo ./Drivers/Vendor/FatFs/source/diskio.d ./Drivers/Vendor/FatFs/source/diskio.o ./Drivers/Vendor/FatFs/source/diskio.su ./Drivers/Vendor/FatFs/source/ff.cyclo ./Drivers/Vendor/FatFs/source/ff.d ./Drivers/Vendor/FatFs/source/ff.o ./Drivers/Vendor/FatFs/source/ff.su ./Drivers/Vendor/FatFs/source/ff_gen_drv.cyclo ./Drivers/Vendor/FatFs/source/ff_gen_drv.d ./Drivers/Vendor/FatFs/source/ff_gen_drv.o ./Drivers/Vendor/FatFs/source/ff_gen_drv.su ./Drivers/Vendor/FatFs/source/ffsystem_baremetal.cyclo ./Drivers/Vendor/FatFs/source/ffsystem_baremetal.d ./Drivers/Vendor/FatFs/source/ffsystem_baremetal.o ./Drivers/Vendor/FatFs/source/ffsystem_baremetal.su

.PHONY: clean-Drivers-2f-Vendor-2f-FatFs-2f-source

