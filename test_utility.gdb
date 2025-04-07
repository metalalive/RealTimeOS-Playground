
define reload_image
    monitor  reset
    monitor  halt
    load
end

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
    print "------- TIM3 registers -------"
    print htim3
    print "below are the first 16 registers, from 0x40000400 to 0x4000043C"
    print /x *(int (*) [8]) htim3.Instance
    print /x *((int (*) [8]) htim3.Instance + 1)
end

define stm32f4_inspect_exti
    print "------- EXTI -------"
    printf "from 0x40013c00 to 0x40013c1c"
    print /x *(int (*) [8]) 0x40013c00
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

define report_test_result
    set print address on
    set $currTestNode = testloggerlist->head
    set $null         = 0
    set $num_of_tests = 0
    set $num_of_fails = 0

    print "------- start of error report -------"
    while ($currTestNode != $null)
        if ($currTestNode->failFlag != 0)
            print  "[file path]: "
            print  $currTestNode->filepath
            print  "[line number]: "
            print  $currTestNode->lineNumber
            print  "[description]: "
            print  $currTestNode->description
            if (($currTestNode->compare_condition | 0x1) == 0x1)
                print  "[compare type]: equal-to"
            end
            if (($currTestNode->compare_condition | 0x2) == 0x1)
                print  "[compare type]: greater-than"
            end
            if (($currTestNode->compare_condition | 0x4) == 0x1)
                print  "[compare type]: less-than"
            end
            if ($currTestNode->compare_condition == 0x0)
                print  "[compare type]: in-range"
            end
            if ($currTestNode->compare_condition == 0x0)
                print  "[range]: (represented as pointer) "
                print  /x  $currTestNode->expectedValue[0]
                print  /x  $currTestNode->expectedValue[1]
            else
                print  "[expected value]: (represented as pointer) "
                print  /x  $currTestNode->expectedValue[0]
            end
            print  "[actual value]: (represented as pointer) "
            print  /x  $currTestNode->actualValue
            print ""
            set $num_of_fails += 1
        end
        set $num_of_tests += 1
        set $currTestNode  = $currTestNode->next
    end
    print "------- end of error report -------"
    print ""
    print "[number of tests]:"
    print $num_of_tests 
    print "[number of failure]:"
    print $num_of_fails
end

define report_mpu_region
    set *0xe000ed98 = $arg0
    x/3xw 0xe000ed98
end

file    build/stm32_port_freertos_v10.2-utest.elf
target  remote localhost:3333
reload_image
break   TestEnd
# stm32f4_breakpoint_setup
info breakpoints

