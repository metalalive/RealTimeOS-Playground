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


Build test image. The commands below should include base path to toolchain e.g. `ARM_TOOLCHAIN_BASEPATH=/PATH/TO/TOOLCHAIN` depending on your target board.
```bash
make UNIT_TEST=yes ;
make INTEGRATION_TEST=yes ;
```

Clean up all built images
```bash
make clean
```

## Load image, Run, and Debug
Launch debug server after interfacing debug hardware to your target board
```bash
make dbg_server
```

Launch debug client
```bash
make dbg_client ARM_TOOLCHAIN_BASEPATH=/PATH/TO/TOOLCAHIN
```
- Remind the debugger to use depends on your target board
- Note you can open `./test_utility.gdb` and modify the image name and path in your case, by modifying the command `file <YOUR_PATH_TO_TEST_IMAGE>` , also set up extra breakpoints for your requirement.


### Test Result Check
To see the unit-test result, type command `report_test_result` in the GDB client console, you can see number of test cases running on the target board, and how many of them failed. 

For example, the text report below shows that we have 36 test cases and none of the tests failed.
```
$1 = "------- start of error report -------"
$2 = "------- end of error report -------"
$3 = ""
$4 = "[number of tests]:"
$5 = 36
$6 = "[number of failure]:"
$7 = 0
```

If you get some tests failed, the report also shows where did the assertion failure happen. In the case below, there is one assertion failure at line 28 of the file `sw_timer.c`, the expected value is stored in RAM address `0x200058e0`, similarly the actual value is stored in RAM address `0x200058f8`, the data type of the expected/actual value depends on what you'd like to check with the test assertion function.
```
$8 = "------- start of error report -------"
$9 = "[file path]: "
$10 = 0x8009848 "Src/tests/integration/FreeRTOS/sw_timer.c"
$11 = "[line number]: "
$12 = 28
$13 = "[description]: "
$14 = 0x8009834 "software timer test"
$15 = "[expected value]: (represented as pointer) "
$16 = 0x200058e0
$17 = "[actual value]: (represented as pointer) "
$18 = 0x200058f8
$19 = ""
$20 = "------- end of error report -------"
$21 = ""
$22 = "[number of tests]:"
$23 = 36
$24 = "[number of failure]:"
$25 = 1
```

For integration test, you can interupt (by pressing keys `Ctrl + C`) at any time to see whether any task has been hitting assertion failure.

