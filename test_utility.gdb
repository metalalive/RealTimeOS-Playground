
define stm32f4_inspect_systick
    print "--- SysTick config ---"
    print /x *(int (*) [4]) 0xe000e010
    print "--- System Control Space (SCS) ---"
    printf "ICSR : 0x%x \n", *(int *) 0xe000ed04
    printf "SCR :  0x%x \n", *(int *) 0xe000ed10
    print "SHPR 1-3 :"
    print /x *(int (*) [3]) 0xe000ed18
    printf "SHCSR :  0x%x \n", *(int *) 0xe000ed24
end

define stm32f4_inspect_nvic
    print "------- NVIC -------"
    printf "NVIC_ISER0 : 0x%x \n",   *(int *) 0xe000e100
    printf "NVIC_ICER0 : 0x%x \n", *(int *) 0xe000e180
    printf "NVIC_ISPR0 : 0x%x \n",  *(int *) 0xe000e200
    printf "NVIC_ICPR0 : 0x%x \n",  *(int *) 0xe000e280
    printf "NVIC_IABR0 : 0x%x \n", *(int *) 0xe000e300
    printf "NVIC_IPR7  : 0x%x \n", *((int *) 0xe000e400 + 8 - 1)
    print "NVIC_ISERx : \n"
    print /x *(int (*) [16]) 0xe000e100
end

define stm32f4_inspect_tim3
    print "------- TIM3 registers -------"
    print htim3
    print "below are the first 16 registers, from 0x40000400 to 0x4000043C"
    print /x *(int (*) [8]) htim3.Instance
    print /x *((int (*) [8]) htim3.Instance + 1)
end

define stm32f4_inspect_exti
    print "------- EXTI -------"
    printf "from 0x40013c00 to 0x40013c1c \n"
    print /x *(int (*) [8]) 0x40013c00
end

define stm32f4_inspect_spi
    print "------- SPI1 -------"
    print "starts from CR1, CR2, SR, DR : \n"
    print /x *(int (*) [4]) 0x40013000
    print "------- SPI2 -------"
    print "starts from CR1, CR2, SR, DR : \n"
    print /x *(int (*) [4]) 0x40003800
end

define stm32f4_inspect_i2c
    print "------- I2C3 -------"
    print "CR1, CR2, OAR1, OAR2, DR, SR1, SR2, CCR : \n"
    print /x *(int (*) [8]) 0x40005c00
    print "TRISE, FLTR : \n"
    print /x *(int (*) [2]) 0x40005c20
end

define stm32f4_inspect_dma
    print "------- DMA1 common -------"
    print "LISR, HISR, LIFCR, HIFCR : \n"
    print /x *(int (*) [4]) 0x40026000
    print "------- DMA1 Stream4 -------"
    print "CR, NDTR, PAR, M0AR, M1AR, FCR : \n"
    print /x *(int (*) [6]) 0x40026070
end

define stm32f4_inspect_misc
    print "------- Reset and Clock -------"
    printf "RCC_APB1ENR : 0x%x \n", *(int *) 0x40023840
    print "------- Power conrtol -------"
    printf "PWR_CR  : 0x%x \n", *(int *) 0x40007000
    printf "PWR_CSR : 0x%x \n", *(int *) 0x40007004
    print "------- Debugger -------"
    printf "DBGMCU_IDCODE : 0x%x \n", *(int *) 0xe0042000
    printf "DBGMCU_CR : 0x%x \n", *(int *) 0xe0042004
    printf "DBGMCU_APB1_FZ : 0x%x \n", *(int *) 0xe0042008
    printf "DBGMCU_APB2_FZ : 0x%x \n", *(int *) 0xe004200c
end

define stm32f4_inspect_regs
    print "------- ARM Cortex-M4 -------"
    print "---- special-purpose registers ----"
    info register
    stm32f4_inspect_systick
    stm32f4_inspect_exti
    stm32f4_inspect_nvic
    stm32f4_inspect_tim3
    stm32f4_inspect_misc
end

define stm32f4_breakpoint_setup
    break  SysTick_Handler
    disable 2
    break  TIM2_IRQHandler
    disable 3
    break  *0x8005734
    break  *0x800573e
end


define report_mpu_region
    set *0xe000ed98 = $arg0
    x/3xw 0xe000ed98
end

file    build/stm32-freertos-v10-itest.elf
target  remote localhost:3333
monitor  reset
monitor  halt
load

# following parameters will be referred in FreeRTOS utility GDB script
set $config_use_posix_errno_enabled = 0
set $cfg_max_priority = 7
set $platform_mem_baseaddr = 0x20000000
set $platform_mem_maxsize = 0x1ffff
source ./Src/os/FreeRTOS/util.gdb

break   TestEnd

# watch  *0xe000e010
# break  Src/tests/unit/FreeRTOS/portable/ARM_CM4_MPU/test_vPortPendSVHandler.c:162
# break  Src/tests/unit/baremetal/stm32f4xx/test_I2C.c:154
# break  I2C_ITError

# stm32f4_breakpoint_setup
info breakpoints

