APP_COMMON_HW_BASEPATH = examples/src/hardware/stm32f4xx

APP_COMMON_C_SOURCES = \
  $(APP_COMMON_HW_BASEPATH)/stm32f4xx_it.c \
  $(APP_COMMON_HW_BASEPATH)/stm32f4xx_hal_msp.c \
  $(APP_COMMON_HW_BASEPATH)/stm32f4xx_hal_config.c \
  $(APP_COMMON_HW_BASEPATH)/system_stm32f4xx.c

APP_COMMON_C_INCLUDES = \
  examples/include/FreeRTOS/v10.2.0 \
  examples/include/hardware/stm32f4xx

APPCFG_ASM_SOURCES = $(APP_COMMON_HW_BASEPATH)/startup_stm32f446xx.s

APP_LINK_SCRIPT = $(APP_COMMON_HW_BASEPATH)/STM32F446RETx_FLASH.ld

