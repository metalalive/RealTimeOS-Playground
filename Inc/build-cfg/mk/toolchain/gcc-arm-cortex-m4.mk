include ./Inc/build-cfg/mk/toolchain/gcc-arm.mk

CPU = -march=armv7e-m+fp  -mcpu=cortex-m4

FPU = -mfpu=fpv4-sp-d16

FLOAT-ABI = -mfloat-abi=hard

GCC_MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

