## Demo Test Application

### Test Result Check
To see the unit-test result:
- build the test application with debug symbols enabled (`DEBUG=1`)
- set up a breakpoint on `TestEnd()` in debuggers like GDB, run application until it reaches the function, check global variable `Unity` for number of test cases done and failed, 

For integration test, you can interupt (by pressing keys `Ctrl + C`) at any time to see whether any task has been hitting assertion failure.

