include  $(APPCFG_PATH)/../hardware/stm32f4xx/build.mk

UTEST_C_SOURCES = \
  FreeRTOS/portable/ARM_CM4_MPU/test_pxPortInitialiseStack.c \
  FreeRTOS/portable/ARM_CM4_MPU/test_vPortPendSVHandler.c   \
  FreeRTOS/portable/ARM_CM4_MPU/test_xPortStartScheduler.c  \
  FreeRTOS/portable/ARM_CM4_MPU/test_xPortRaisePrivilege.c  \
  FreeRTOS/portable/ARM_CM4_MPU/test_vPortEnterCritical.c   \
  FreeRTOS/portable/ARM_CM4_MPU/test_vPortStoreTaskMPUSettings.c  \
  FreeRTOS/portable/ARM_CM4_MPU/test_vPortSysTickHandler.c  \
  FreeRTOS/portable/ARM_CM4_MPU/test_TicklessIdleSleep.c \
  baremetal/stm32f4xx/test_AnalogDigitalCvt.c \
  baremetal/stm32f4xx/test_UART.c \
  baremetal/stm32f4xx/test_SPI.c \
  baremetal/stm32f4xx/test_I2C.c \
  Unity/src/unity.c \
  Unity/extras/fixture/src/unity_fixture.c \
  FreeRTOS/test_runner.c \
  FreeRTOS/entry.10.2.c

APPCFG_C_SOURCES = \
  $(APP_COMMON_C_SOURCES) \
  $(addprefix $(APPCFG_PATH)/, $(UTEST_C_SOURCES))

UTEST_C_INCLUDES = \
  Unity/src \
  Unity/extras/fixture/src

APPCFG_C_INCLUDES = \
  $(APP_COMMON_C_INCLUDES) \
  $(addprefix $(APPCFG_PATH)/, $(UTEST_C_INCLUDES))

