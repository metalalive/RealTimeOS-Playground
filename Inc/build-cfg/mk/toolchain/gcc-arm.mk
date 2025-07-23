TOOLCHAIN_BASEPATH ?= /PATH/TO/YOUR_TOOLCHAIN

#######################################
# paths
#######################################
EXTRA_BINPATH  = $(TOOLCHAIN_BASEPATH)/bin
EXTRA_BINPATH2 = $(TOOLCHAIN_BASEPATH)/libexec/gcc/arm-none-eabi/14.2.1
EXTRA_BINPATH3 = $(TOOLCHAIN_BASEPATH)/arm-none-eabi/bin

# The gcc compiler bin path can be either defined in make command via
# either it can be added to the PATH environment variable.
export PATH := $(EXTRA_BINPATH):$(EXTRA_BINPATH2):$(EXTRA_BINPATH3):$(PATH)

EXTRA_LIBPATH = $(TOOLCHAIN_BASEPATH)/arm-none-eabi/lib
EXTRA_LIBPATH2 = $(TOOLCHAIN_BASEPATH)/libexec/gcc/arm-none-eabi/14.2.1
EXTRA_LIBPATH3 = $(TOOLCHAIN_BASEPATH)/lib/gcc/arm-none-eabi/14.2.1
export LD_LIBRARY_PATH := $(EXTRA_LIBPATH):$(EXTRA_LIBPATH2):$(EXTRA_LIBPATH3):$(LD_LIBRARY_PATH)


TOOLCHAIN_INCLUDES = \
  $(TOOLCHAIN_BASEPATH)/lib/gcc/arm-none-eabi/14.2.1/include \
  $(TOOLCHAIN_BASEPATH)/arm-none-eabi/include

GNU_CMD_PREFIX = arm-none-eabi-

DBG_CUSTOM_SCRIPT_PATH = ./test_utility.gdb

dbg_client:
	## @gdb-multiarch -x ./test_utility.gdb
	@$(GNU_CMD_PREFIX)gdb -x $(DBG_CUSTOM_SCRIPT_PATH)

