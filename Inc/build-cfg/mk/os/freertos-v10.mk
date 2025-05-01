
OS_C_SOURCES = \
  Src/os/FreeRTOS/v10.2.0/portable/MemMang/heap_4.c \
  Src/os/FreeRTOS/v10.2.0/croutine.c      \
  Src/os/FreeRTOS/v10.2.0/event_groups.c  \
  Src/os/FreeRTOS/v10.2.0/list.c          \
  Src/os/FreeRTOS/v10.2.0/queue.c         \
  Src/os/FreeRTOS/v10.2.0/stream_buffer.c \
  Src/os/FreeRTOS/v10.2.0/tasks.c         \
  Src/os/FreeRTOS/v10.2.0/timers.c

OS_C_INCLUDES = Src/os/FreeRTOS/v10.2.0/include

ifeq ($(HW_CPU), arm-cortex-m4)

OS_C_SOURCES += Src/os/FreeRTOS/v10.2.0/portable/GCC/ARM_CM4_MPU/port.c
OS_C_INCLUDES += Src/os/FreeRTOS/v10.2.0/portable/GCC/ARM_CM4_MPU

else

$(error HW_CPU is not set! Use any hardware platform which sets that parameter, e.g. HW_PLATFORM=stm32f446)

endif
