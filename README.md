# Real-Time OS Playground
This repository is for integration experiment with variety of hardware platforms and real-time operating systems

### Supported platforms
|device|CPU|RTOS|
|------|---|----|
|STM32F446RE Nucleo|ARM Cortex-M4|[FreeRTOS v10.2.0](https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/V10.2.0)|

## Build
### Prerequisite
|name|version|description|
|----|-------|-----------|
|[ARM GNU toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)| 14.2 release 1 |for cross-compiling and debugging ARM CPU dev board|
|[OpenOCD](https://openocd.org/)| 0.12.0 |local server for debugging target dev board|

### Optional Dependencies
|name|version|description|
|----|-------|-----------|
|[ST-Link Tool](https://www.st.com/en/development-tools/stsw-link004.html)| 1.8.0 |flashing tools for STM32 dev boards|

### Key Build Parameters
Build your application with the command `make startbuild`, that requires the following parameters:

##### `HW_PLATFORM` (required)
- Selects the hardware config file `./Inc/build-cfg/mk/hw/$(HW_PLATFORM).mk`.
- Supported values include `stm32f446`.

##### `OS` (required)
- Selects the config for the real-time OS `./Inc/build-cfg/mk/os/$(OS).mk`.
- Supported values include `freertos-v10`.

##### `APPCFG_PATH` (required)
- Path to your application's top-level `build.mk`.
- This file must define `APP_C_SOURCES`, `APPCFG_C_INCLUDES`, etc.

##### `TOOLCHAIN_BASEPATH` (required)
Path to the cross-compile toolchain you use.

##### `APP_NAME`
Name of the successfully built image, defaults to `app`.

##### `DEBUG`
Set to `1` to turn on debug symbols, or `0` (or omit) to turn them off.

#### Example Build Command
```bash
make startbuild \
    HW_PLATFORM=stm32f446  OS=freertos-v10   DEBUG=1 \
    APPCFG_PATH=$PWD/examples/src/integration/FreeRTOS/  APP_NAME=helloworld  \
    TOOLCHAIN_BASEPATH=/PATH/TO/GCC/INSTALLED
```

All outputs are placed under the build directory (default: `build/`) and include:

- `build/$(APP_NAME).elf` , linked ELF binary
- `build/$(APP_NAME).hex` , Intel HEX file
- `build/$(APP_NAME).bin` , raw binary
- `build/$(APP_NAME).text` , human-readable dump


## Other Available Commands
Clean up all built images
```bash
make clean
```

Launches a debug server for a client debugger to connect. OpenOCD is used here.   
```bash
make dbg_server
```

Launches the debugger client to load the image, set breakpoints, and watchpoints for execution. The debugger depends on the CPU vendor/architecture.  
```bash
make dbg_client TOOLCHAIN_BASEPATH=/PATH/TO/GCC/INSTALLED
```
- The debugger to use depends on your target board
- You can modify path to the image, breakpoints, and watchpoints ... etc. in `./test_utility.gdb` based on your requirement.

Shows this helper document
```bash
make help
```

