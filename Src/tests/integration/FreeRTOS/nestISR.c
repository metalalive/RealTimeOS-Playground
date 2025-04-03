#include "tests/integration/FreeRTOS/nestISR.h"

// external routines / variables --------------------------
// internal variables -------------------------------------

static volatile unsigned int uNumNestInterrupts;
// uMaxNestInterrupts is only used for recording maximum value of uNumNestInterrupts
// discovered in vTIM4regressionFromISR()
static volatile unsigned int uMaxNestInterrupts;

void vSetupNestedInterruptTest() {
    uNumNestInterrupts = 0;
    uMaxNestInterrupts = 0;
    vIntegrationTestDeviceInit();
}

void vNestInterruptTestISR1(void) {
    float reg_read = 0.0f;
    const float EXPECTED_VALUE = 2.7181f;
    uNumNestInterrupts++;
    vFloatRegSetTest(EXPECTED_VALUE);
    vPreemptCurrentInterruptTest();
    reg_read = fFloatRegGetTest();
    // error happened if the values are different
    TEST_ASSERT_EQUAL_FLOAT(EXPECTED_VALUE, reg_read);
    uNumNestInterrupts--;
}

void vNestInterruptTestISR2(void) {
    const portSHORT MAX_NUM_NEST_INTERRUPT = 2;
    const float     EXPECTED_VALUE = 3.1415f;
    uNumNestInterrupts++;
    if(uMaxNestInterrupts < uNumNestInterrupts) {
        uMaxNestInterrupts = uNumNestInterrupts;
    }
    TEST_ASSERT_LESS_OR_EQUAL_UINT(MAX_NUM_NEST_INTERRUPT, uMaxNestInterrupts);
    vFloatRegSetTest(EXPECTED_VALUE);
    uNumNestInterrupts--;
}

void vNestInterruptTestTickHook(void) {
    vFloatRegSetTest(0.377f);
}
