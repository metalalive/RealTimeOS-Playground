include  $(APPCFG_PATH)/../../hardware/stm32f4xx/build.mk

ITEST_C_SOURCES = \
  test_entry.c  \
  port/stm32f446.c \
  nestISR.c  \
  integer.c  \
  dynamic_case1.c  \
  dynamic_case2.c  \
  block_time.c  \
  queue_case1.c \
  queue_case2.c \
  queue_case3.c \
  semphr_bin_case1.c  \
  semphr_bin_case2.c  \
  semphr_cnt.c  \
  mutex_case1.c \
  recur_mutex.c \
  notify.c \
  sw_timer.c  \
  stack_ovfl_chk.c \
  test_runner.c


APPCFG_C_SOURCES = $(APP_COMMON_C_SOURCES)  $(addprefix $(APPCFG_PATH)/, $(ITEST_C_SOURCES))

ITEST_C_INCLUDES = Inc/tests/integration/FreeRTOS

APPCFG_C_INCLUDES = $(APP_COMMON_C_INCLUDES)  $(ITEST_C_INCLUDES)

