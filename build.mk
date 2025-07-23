
APP_NAME ?= app
BUILD_DIR ?= build
DEBUG ?= 1
RTOS_HW_BUILD_PATH = $(shell pwd)

# Allow selecting a hardware-specific config file via
# HW_PLATFORM (no extension) e.g. make HW_PLATFORM=stm32f446
ifdef HW_PLATFORM
include ./Inc/build-cfg/mk/hw/$(HW_PLATFORM).mk
else
$(error HW_PLATFORM is not set! Use e.g. make HW_PLATFORM=stm32f446)
endif

$(info HW_PLATFORM is: $(HW_PLATFORM))
$(info HW_CPU is: $(HW_CPU))
$(info OS is: $(OS))
$(info BUILD_DIR is: $(BUILD_DIR))

ifdef OS
include ./Inc/build-cfg/mk/os/$(OS).mk
else
$(error parameter OS is not set! Use e.g. make OS=freertos-v10)
endif

include ./Inc/build-cfg/mk/toolchain/gcc-$(HW_CPU).mk

ifdef APPCFG_PATH
include  $(APPCFG_PATH)/build.mk
else
$(error parameter APPCFG_PATH is missing! Use e.g. make APPCFG_PATH=/PATH/TO/YOUR/APP/PROJECT_HOME)
endif

# TODO, parameterize gcc generic options, in case to
# support other build tools , e.g. cargo

#######################################
# binaries
#######################################
CC = $(GNU_CMD_PREFIX)gcc
AS = $(GNU_CMD_PREFIX)gcc -x assembler-with-cpp
AR = $(GNU_CMD_PREFIX)ar
CP = $(GNU_CMD_PREFIX)objcopy
SZ = $(GNU_CMD_PREFIX)size
DUMP = $(GNU_CMD_PREFIX)objdump
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
######################################
# macros for gcc
######################################
AS_DEFS = 

C_DEFS = $(HW_C_DEFS) $(OS_C_DEFS) $(addprefix -D, $(APPCFG_C_DEFS))

######################################
# sources
######################################
C_SOURCES = $(OS_C_SOURCES) $(HW_C_SOURCES)

ASM_SOURCES = $(HW_ASM_SOURCES) $(APPCFG_ASM_SOURCES)

######################################
# headers
######################################

AS_INCLUDES = 

C_HEADER_PATHS = Inc  $(HW_C_INCLUDES)  $(OS_C_INCLUDES) \
			 $(APPCFG_C_INCLUDES)  $(TOOLCHAIN_INCLUDES) 

C_INCLUDES = $(addprefix -I,$(C_HEADER_PATHS))

#######################################
# Compile Flags
#######################################
# optimization
OPT = -Og

# compile gcc flags
CFLAGS = $(GCC_MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -Wint-to-pointer-cast
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# loading / linking flags
#######################################
# link script
LDSCRIPT = $(APP_LINK_SCRIPT)

# libraries
LIBS = -lc -lm -lnosys
LIBDIR = 
LDFLAGS = $(GCC_MCU) -specs=nosys.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

ifeq ($(DEBUG), 1)
LDFLAGS += -g -gdwarf-2
endif


#######################################
# Targets
#######################################
# default action: build all
all: startbuild  $(BUILD_DIR)/$(APP_NAME).hex  $(BUILD_DIR)/$(APP_NAME).bin

#######################################
# build the application
#######################################
# list of objects
OBJS4LIB = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
vpath %.c $(dir $(C_SOURCES))

# list of ASM program objects
OBJS4LIB += $(addprefix $(BUILD_DIR)/, $(ASM_SOURCES:.s=.o))
vpath %.s $(dir $(ASM_SOURCES))

OBJS4APP = $(addprefix $(BUILD_DIR)/, $(APPCFG_C_SOURCES:.c=.o))
vpath %.c $(sort $(dir $(APPCFG_C_SOURCES)))

LIB_DIRS := $(dir $(APPCFG_LIBS_PATHS))
LIB_FILES := $(notdir $(APPCFG_LIBS_PATHS))
LIB_NAMES := $(patsubst lib%,%, $(basename $(LIB_FILES)))
LIBS4APP := \
    $(foreach idx,$(shell seq 1 $(words $(LIB_DIRS))), \
        -L$(word $(idx),$(LIB_DIRS)) -l$(word $(idx),$(LIB_NAMES)) \
    )

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(<:.c=.lst) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(AS) -c $(CFLAGS) $< -o $@

# Some drivers provide weak functions which let application programs replace with,
# static linking always consider default weak function over other strong functions
# with the same signature.
# 
# Because of that, it is not suitable to build such functions to (shared / static)
# library.in this project
#
# Be sure re-declared functions in application code can overwrite the
# weak functions in `C_SOURCES` and `ASM_SOURCES`

$(BUILD_DIR)/$(APP_NAME).elf: $(APPCFG_LIBS_PATHS) $(OBJS4APP) $(OBJS4LIB)
	$(CC) $(LDFLAGS)  $(OBJS4APP) $(OBJS4LIB) $(LIBS4APP) -o $@
	$(SZ) $@

startbuild: $(BUILD_DIR)/$(APP_NAME).text

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR)/%.text: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(DUMP) -Dh $< > $@

$(BUILD_DIR):
	mkdir $@

