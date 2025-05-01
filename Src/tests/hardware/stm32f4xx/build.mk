
APP_COMMON_C_SOURCES = \
  Src/tests/hardware/stm32f4xx/stm32f4xx_it.c \
  Src/tests/hardware/stm32f4xx/stm32f4xx_hal_msp.c \
  Src/tests/hardware/stm32f4xx/stm32f4xx_hal_config.c \
  Src/tests/hardware/stm32f4xx/system_stm32f4xx.c

APP_COMMON_C_INCLUDES = \
  Inc/tests/FreeRTOS/v10.2.0 \
  Inc/tests/hardware/stm32f4xx

APPCFG_ASM_SOURCES = Src/tests/hardware/stm32f4xx/startup_stm32f446xx.s

APP_LINK_SCRIPT = Src/tests/hardware/stm32f4xx/STM32F446RETx_FLASH.ld

