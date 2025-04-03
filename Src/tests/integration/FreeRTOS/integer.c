#include "tests/integration/FreeRTOS/integer.h"

#define NUM_OF_TASKS         4
// in our case, we create tasks' stack using privileged function  pvPortMalloc() 
// implemented in heap_4.c, each allocated block memory preserves first 8 bytes 
// for  internal data structure, representing size of the allocated block.
#define intgSTACK_SIZE       ( ( unsigned portSHORT ) 0x3e )


#pragma GCC push_option
#pragma GCC optimize ("O0")
static void vCompetingIntMathTask1(void *pvParams) {
    // can be any number for test, be aware of overflow issues,
    // Note: there are not many target platforms that support 64-bit execution environment ...
    const int32_t loperands[4] = {7456,   1234567,  -101,  -918};
    const int32_t expectedVal  = (7456 + (1234567 / -101)) * -918; 
    int32_t       actualVal    = 0;
    const int32_t *loperandsp = loperands;
    for(;;) {
        actualVal  = (*(loperandsp+0) + (*(loperandsp+1) / *(loperandsp+2))) * *(loperandsp+3);
        // temporarily give up the CPU control, 
        // later on it will come back to see the calculation result is still as expected.
        taskYIELD();
        TEST_ASSERT_EQUAL_INT32(expectedVal, actualVal);
        actualVal  = 0;
    }
}
#pragma GCC pop_option


static void vCompetingIntMathTask2(void *pvParams) {
    // in this part of the test, we create both of privileged task & unprivileged task
    // to run this function, since unprivileged task is NOT allowed to call the 
    // privileged function pvPortMalloc(), therefore
    // we statically declare an integer array = {1, 2, ...., 29, 30}, 
    // instead of using pvPortMalloc()
    #define  TEST_ARRAY_LENGTH  30
    portINT expectedVal = 0L, actualVal = 0L;
    portSHORT idx = 0;

    // note that pvPortMalloc can be called only in privileged task. In unprivileged
    // task the array is internally declared, instead of using pvPortMalloc(), GCC
    // compiler should assign from the allocated stack for the internal array. 
    portINT larray[TEST_ARRAY_LENGTH] = {0};
    // Then we access the array by the pointer, in order to prevent some compilers from
    // producing base address (of memory access instructions) going out of the defined 
    // stack region, MPU (for some CPUs) might treat this as invalid access.
    // 
    // e.g. 
    // The stack region (the address range: 0x300 - 0x400) is assigned to an unprivileged
    // task, some compilers might generate instruction sequence like following :
    //
    //     add  r4, r5, #0x80
    //     ldr  r0, [r4, #-0x38]
    //     ......
    //
    // where r5 is 0x384 (or greater than 0x384).
    // r4 will be 0x404 and immediately fed to the "base address" of next load instruction,
    // the actual address to memory location will be 0x404 - 0x78 = 0x3cc , which is lesser
    // than the upper bound of allocated stack space (0x400 in this case) , therefore this
    // load access should be completed without any problem.
    // However in some CPU implementation, after MPU is enabled and CPU comes to the load 
    // instruction, it only checks boundary of the "base address" before accessing the memory
    // (in this case, CPU might only check 0x404 and find it greater than 0x400, therefore
    //  the CPU generates memory-related fault, regardless of the negative constant immediately
    //  after the base address)
    //  
    portINT  *larrayp = larray;
    for(idx=0; idx<TEST_ARRAY_LENGTH; idx++) {
        *(larrayp + idx) = (idx + 1) * 3;
         expectedVal += *(larrayp + idx);
    }
    taskYIELD();
    for(;;) {
        actualVal  = 0L;
        for(idx=0; idx<TEST_ARRAY_LENGTH; idx++) {
            actualVal += *(larrayp + idx);
        }
        // temporarily give up the CPU control, later on after context switches,
        // CPU will come back to see the calculation result is still as expected.
        taskYIELD();
        TEST_ASSERT_EQUAL_INT32(expectedVal, actualVal);
    }
    #undef TEST_ARRAY_LENGTH 
} // end of vCompetingIntMathTask2


void vStartIntegerMathTasks( UBaseType_t uxPriority )
{
    extern  UBaseType_t  __unpriv_data_start__ [];
    extern  UBaseType_t  __unpriv_data_end__ [];
    const char           tasknames[NUM_OF_TASKS][12] = {
        "IntMath1up", "IntMath2up", "IntMath1p", "IntMath2p",
    };
    const TaskFunction_t taskFuncs[NUM_OF_TASKS] = {
        vCompetingIntMathTask1, vCompetingIntMathTask2,
        vCompetingIntMathTask1, vCompetingIntMathTask2
    };
    const UBaseType_t taskPriority[NUM_OF_TASKS] = {
        (UBaseType_t) (uxPriority),
        (UBaseType_t) (uxPriority),
        (UBaseType_t) (uxPriority | portPRIVILEGE_BIT),
        (UBaseType_t) (uxPriority | portPRIVILEGE_BIT),
    };
    const portSHORT  stackSize[NUM_OF_TASKS] = { (intgSTACK_SIZE), (intgSTACK_SIZE), (intgSTACK_SIZE), (intgSTACK_SIZE) };
    StackType_t     *stackMemSpace[NUM_OF_TASKS] = {0};
    BaseType_t    xState; 
    portSHORT     idx, jdx;

    // allocate stack memory for each task
    // Note that for some CPU implementation, you must meet the requirement while applying MPU
    // to limit unprivileged memory accesses, such as max/min size restriction, address alingment
    // ...... etc.
    for(idx=0; idx<NUM_OF_TASKS; idx++) {
        stackMemSpace[idx] = (StackType_t *) pvPortMalloc(sizeof(StackType_t) * stackSize[idx]);
    }
    for(idx=0; idx<NUM_OF_TASKS; idx++) {
        // collect all necessary information to the following structure feeding to xTaskCreate()
        TaskParameters_t tskparams = {
            taskFuncs[idx], tasknames[idx], stackSize[idx], (void *) NULL,
            taskPriority[idx], stackMemSpace[idx], // leave MPU regions uninitialized
        }; 
        for(jdx=0; jdx<portNUM_CONFIGURABLE_REGIONS; jdx++) {
            tskparams.xRegions[jdx].pvBaseAddress   = NULL;
            tskparams.xRegions[jdx].ulLengthInBytes = 0;
            tskparams.xRegions[jdx].ulParameters    = 0;
        } // default value to xRegions 
        // add extra region for logging assertion failure in unprivileged tasks.
        if((taskPriority[idx] & portPRIVILEGE_BIT) != portPRIVILEGE_BIT) {
            tskparams.xRegions[0].pvBaseAddress   = (void *) __unpriv_data_start__;
            tskparams.xRegions[0].ulLengthInBytes = (UBaseType_t) __unpriv_data_end__ - (UBaseType_t) __unpriv_data_start__;
            tskparams.xRegions[0].ulParameters    = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk
                                                   | MPU_RASR_C_Msk | MPU_RASR_B_Msk;
        }
        xState = xTaskCreateRestricted( (const TaskParameters_t * const)&tskparams, NULL );
        configASSERT( xState == pdPASS );
    } // end of loop
} // end of vStartIntegerMathTasks
