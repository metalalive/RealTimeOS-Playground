#include <stdlib.h>
#include <string.h>
// for C unit test framework Unity
#include "unity_fixture.h"

#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#define TEST_STACK_SIZE  0x60
#define TEST_FLOAT_POINT_ENABLE   1
#define TEST_FLOAT_POINT_DISABLE  0
#define NUM_BASIC_FRAMES    10
#define NUM_EXT_FP_FRAMES   17

extern volatile void *pxCurrentTCB;
// in this test, task A-one is a mirror of main-thread task,
// it does not do anything but copy part of stack from main thread test-run function
static TaskHandle_t taskAOneHandle = NULL;
static TaskHandle_t taskATwoHandle = NULL;

// Variable to save the initial context pointer for task A-one.
// Compare it after the context switch to verify that its context was saved.
static StackType_t *xTaskOneStack = NULL;
static StackType_t *xTaskTwoStack = NULL;

// bit 0, PendSV handler
// bit 1, task A-two
unsigned portSHORT otherFuncsReached;

// To avoid our GCC compiler from automatically choosing GPR r0, r4-r12 as operands of
// subsequent instructions, following operations should be done in the inline assembly :
// - set PendSV bit in system control block
// - switch between main stack pointer and process stack pointer in main thread task
//
__attribute__((always_inline)) __INLINE void vMockTaskYield(void) {
    __asm volatile(
         "ldr  r3,=%0     \n"
         "mov  r2, #1     \n"
         "lsl  r2, r2, %1 \n"
         "str  r2, [r3, #4] \n" // SCB->ICSR = 1 << SCB_ICSR_PENDSVSET_Pos
         "dsb  \n" 
         "isb  \n"
        ::"i"(SCB_BASE), "i"(SCB_ICSR_PENDSVSET_Pos)
        : "memory" 
    );
}
__attribute__((always_inline)) __INLINE void vSwitchFromMSP2PSP(StackType_t *sp) {
    __asm volatile (
        "push {r8}          \n"
        "msr  psp, %0       \n"
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x2 \n"
        "msr  control, r8   \n"
        ::"r"(sp):
    );
}
__attribute__((always_inline)) __INLINE void vSwitchFromPSP2MSP(void) {
    __asm volatile (
        "mrs  r8 , control  \n"
        "bic  r8 , r8, #0x2 \n"
        "msr  control, r8   \n"
        "pop  {r8}          \n"
    );
}

static void vTaskFunction_AOne(void *pvParameters) {
    configASSERT(pdFALSE);
}
static void vTaskFunction_ATwo(void *pvParameters) {
    TaskStatus_t taskInfo = {0};
    otherFuncsReached |= (0x1 << 1);
    vTaskGetInfo(taskATwoHandle, &taskInfo, pdFALSE, eInvalid);
    configASSERT(taskInfo.eCurrentState == eRunning);
    vTaskGetInfo(taskAOneHandle, &taskInfo, pdFALSE, eInvalid);
    configASSERT(taskInfo.eCurrentState == eReady);
    vMockTaskYield();
    configASSERT(pdFALSE);
}

__attribute__((naked)) void TEST_HELPER_vPortPendSVHandler_PendSVentry(void) {
    // use r2, r3 to set flag in otherFuncsReached
    // this test is specific to GCC
    __asm volatile (
        "push     {r2-r3} \n"
    );
    __asm volatile (
        "ldrh     r2, [%0, #0] \n"
        "orr.w    r2, r2, #1 \n"
        "strh     r2, [%0, #0] \n"
        "pop      {r2-r3} \n"
        ::"r"(&otherFuncsReached):
    );
    __asm volatile (
        "b  vPortPendSVHandler  \n"
    );
}


TEST_GROUP(PortPendSVHandler);

TEST_SETUP(PortPendSVHandler) {
    xTaskOneStack = unity_malloc(sizeof(StackType_t) * TEST_STACK_SIZE);
    xTaskTwoStack = unity_malloc(sizeof(StackType_t) * TEST_STACK_SIZE);
    const TaskParameters_t xTaskOneParams = {
        .pvTaskCode = vTaskFunction_AOne,
        .pcName = "A-one",
        .usStackDepth = TEST_STACK_SIZE,
        .pvParameters = NULL,
        .uxPriority = tskIDLE_PRIORITY + 1,
        .puxStackBuffer = xTaskOneStack,
        /* MPU related parameters would be set here if needed */
    };
    BaseType_t ret = xTaskCreateRestricted(&xTaskOneParams, &taskAOneHandle);
    configASSERT(ret == pdPASS);
    const TaskParameters_t xTaskTwoParams = {
        .pvTaskCode = vTaskFunction_ATwo,
        .pcName = "A-two",
        .usStackDepth = TEST_STACK_SIZE,
        .pvParameters = NULL,
        .uxPriority = portPRIVILEGE_BIT | (tskIDLE_PRIORITY + 1),
        .puxStackBuffer = xTaskTwoStack,
    };
    ret = xTaskCreateRestricted(&xTaskTwoParams, &taskATwoHandle);
    configASSERT(ret == pdPASS);
    pxCurrentTCB   = NULL;
    otherFuncsReached = 0;
} // end of test-setup

TEST_TEAR_DOWN(PortPendSVHandler) {
    vTaskDelete(taskAOneHandle);
    vTaskDelete(taskATwoHandle);
    pxCurrentTCB = NULL;
    taskAOneHandle = NULL;
    taskATwoHandle = NULL;
    unity_free(xTaskOneStack);
    unity_free(xTaskTwoStack);
    xTaskOneStack = NULL;
    xTaskTwoStack = NULL;
}

TEST( PortPendSVHandler , cs_with_fp ) {
    TaskStatus_t taskInfo = {0};
    // assume task A-one is the currently running in RTOS.
    vTaskSwitchContext();
    TEST_ASSERT_EQUAL_PTR(pxCurrentTCB, (void *) taskAOneHandle);
    
    float expect_fp_vals[15] = {
        10.5f, 11.75f, 12.5f, 13.75f, 14.25f, 15.5f, 16.75f,
        17.0f, 18.5f, 19.25f, 20.5f, 21.0f, 22.25f, 23.5f, 24.75f,
    };
    float actual_fp_vals[8] = {0};
    __asm volatile (
        "push  {r0} \n"
        "mov   r0, %[addr]   \n"
        "vldmia r0!, {s10-s24} \n" // read floats into s10..s24
        "pop   {r0} \n"
        :: [addr] "r" (expect_fp_vals)
        : "r0", "memory"
    );
    size_t num_stack_preserved = NUM_BASIC_FRAMES + NUM_EXT_FP_FRAMES;
    StackType_t *TaskOneStackPtr = &xTaskOneStack[TEST_STACK_SIZE - num_stack_preserved];
    vSwitchFromMSP2PSP(TaskOneStackPtr); // mock task A-one execution using SP_process
    vMockTaskYield();
    vSwitchFromPSP2MSP(); // recover
    // FIXME, expect_fp_vals[0] is modified unexpectedly after switching back
    
    TEST_ASSERT_EQUAL_UINT16( otherFuncsReached, 0x3 );

    TEST_ASSERT_EQUAL_PTR(pxCurrentTCB, (void *) taskAOneHandle);
    vTaskGetInfo(taskAOneHandle, &taskInfo, pdFALSE, eInvalid);
    TEST_ASSERT_EQUAL_UINT32(taskInfo.eCurrentState, eRunning);
    __asm volatile (
        "vstmia %0, {s11-s19}"
        :
        : "r" (actual_fp_vals)
        : "memory"
    );
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(&expect_fp_vals[1], actual_fp_vals, 8);
} // end of test body


TEST(PortPendSVHandler, cs_without_fp) {
    TaskStatus_t taskInfo = {0};
    BaseType_t  rsved[7] = {11, 234, 5, 6789, 10, 2, 345};
    // assume task A-one is the currently running in RTOS.
    vTaskSwitchContext();
    TEST_ASSERT_EQUAL_PTR(pxCurrentTCB, (void *) taskAOneHandle);
    vTaskGetInfo(taskAOneHandle, &taskInfo, pdFALSE, eInvalid);
    TEST_ASSERT_EQUAL_UINT32(taskInfo.eCurrentState, eRunning);
    vTaskGetInfo(taskATwoHandle, &taskInfo, pdFALSE, eInvalid);
    TEST_ASSERT_EQUAL_UINT32(taskInfo.eCurrentState, eReady);
    
    size_t num_stack_preserved = NUM_BASIC_FRAMES;
    StackType_t *TaskOneStackPtr = &xTaskOneStack[TEST_STACK_SIZE - num_stack_preserved];
    vSwitchFromMSP2PSP(TaskOneStackPtr); // mock task A-one execution using SP_process
    vMockTaskYield();
    vSwitchFromPSP2MSP(); // recover
    
    TEST_ASSERT_EQUAL_UINT16( otherFuncsReached, 0x3 );

    vTaskGetInfo(taskAOneHandle, &taskInfo, pdFALSE, eInvalid);
    TEST_ASSERT_EQUAL_UINT32(taskInfo.eCurrentState, eRunning);
    vTaskGetInfo(taskATwoHandle, &taskInfo, pdFALSE, eInvalid);
    TEST_ASSERT_EQUAL_UINT32(taskInfo.eCurrentState, eReady);
    
    BaseType_t  expect_rsv[7] = {11, 234, 5, 6789, 10, 2, 345};
    TEST_ASSERT_EQUAL_INT_ARRAY(rsved, expect_rsv, 7);
} // end of test body

