#include "nestISR.h"

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
    const float EXPECTED_VALUE = 2.7181f;
    uNumNestInterrupts++;
    vFloatRegSetTest(EXPECTED_VALUE);
    vPreemptCurrentInterruptTest();
    float reg_read = fFloatRegGetTest();
    // TODO, handle precision errors
    configASSERT(EXPECTED_VALUE == reg_read);
    uNumNestInterrupts--;
}

void vNestInterruptTestISR2(void) {
    const portSHORT MAX_NUM_NEST_INTERRUPT = 2;
    const float     EXPECTED_VALUE = 3.1415f;
    uNumNestInterrupts++;
    if(uMaxNestInterrupts < uNumNestInterrupts) {
        uMaxNestInterrupts = uNumNestInterrupts;
    }
    configASSERT(MAX_NUM_NEST_INTERRUPT >= uMaxNestInterrupts);
    vFloatRegSetTest(EXPECTED_VALUE);
    uNumNestInterrupts--;
}

void vNestInterruptTestTickHook(void) {
    vFloatRegSetTest(0.377f);
}
