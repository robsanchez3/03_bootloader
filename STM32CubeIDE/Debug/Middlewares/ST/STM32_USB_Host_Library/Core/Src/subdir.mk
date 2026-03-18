################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.c \
C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.c \
C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.c \
C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.c 

OBJS += \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.o \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.o \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.o \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.o 

C_DEPS += \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.d \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.d \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.d \
./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.o: C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.c Middlewares/ST/STM32_USB_Host_Library/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../FATFS/App -I../../FATFS/Target -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/FatFs/source/drivers/USB -I../../USB_Host/Target -I../../USB_Host/App -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.o: C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.c Middlewares/ST/STM32_USB_Host_Library/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../FATFS/App -I../../FATFS/Target -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/FatFs/source/drivers/USB -I../../USB_Host/Target -I../../USB_Host/App -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.o: C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.c Middlewares/ST/STM32_USB_Host_Library/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../FATFS/App -I../../FATFS/Target -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/FatFs/source/drivers/USB -I../../USB_Host/Target -I../../USB_Host/App -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.o: C:/Roberto/O3/O3_Bootloader/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.c Middlewares/ST/STM32_USB_Host_Library/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U599NIHxQ -DSTM32 -DSTM32U5 -DSTM32U599xx -DUSE_HAL_DRIVER -c -I../Inc -I../../Boot/Inc -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../../FATFS/App -I../../FATFS/Target -I../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc -I../../Drivers/Vendor/FatFs/source -I../../Drivers/Vendor/FatFs/source/drivers/USB -I../../USB_Host/Target -I../../USB_Host/App -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STM32_USB_Host_Library-2f-Core-2f-Src

clean-Middlewares-2f-ST-2f-STM32_USB_Host_Library-2f-Core-2f-Src:
	-$(RM) ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.cyclo ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.d ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.o ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.su ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.cyclo ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.d ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.o ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.su ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.cyclo ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.d ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.o ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.su ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.cyclo ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.d ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.o ./Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.su

.PHONY: clean-Middlewares-2f-ST-2f-STM32_USB_Host_Library-2f-Core-2f-Src

