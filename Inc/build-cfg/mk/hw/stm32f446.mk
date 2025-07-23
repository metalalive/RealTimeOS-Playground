
HW_C_SOURCES = \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
  Src/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c

HW_C_INCLUDES = \
  $(RTOS_HW_BUILD_PATH)/Src/drivers/STM32F4xx_HAL_Driver/Inc \
  $(RTOS_HW_BUILD_PATH)/Src/drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
  $(RTOS_HW_BUILD_PATH)/Src/drivers/CMSIS/Device/ST/STM32F4xx/Include \
  $(RTOS_HW_BUILD_PATH)/Src/drivers/CMSIS/Include

HW_ASM_SOURCES = 

HW_C_DEFS = -DUSE_HAL_DRIVER  -DSTM32F446xx 

HW_CPU = arm-cortex-m4

