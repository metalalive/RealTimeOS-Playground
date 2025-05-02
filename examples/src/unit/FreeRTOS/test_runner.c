// for C unit test framework Unity
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "task.h"

// -------------------------------------------------------------------
// this file collects all the test cases of FreeRTOS port functions

// from test_vPortPendSVHandler.c
extern void TEST_HELPER_vPortPendSVHandler_PendSVentry(void);
// from test_xPortStartScheduler.c
extern void TEST_HELPER_backupMSP( void );
extern void TEST_HELPER_restoreMSP( void );
extern void TEST_HELPER_StartScheduler_IncreSysTick(void);
// from test_vPortSuppressTicksAndSleep.c
extern void TEST_HELPER_SuppressTickSleep_IncrSysTick(void);
// from test_vPortEnterCritical.c
extern void TEST_HELPER_EnterCritical_SVCentry( void );
extern void TEST_HELPER_EnterCritical_hardFaultEntry(UBaseType_t *sp);
extern void TEST_HELPER_EnterCritical_sysTickEntry( void );
//from test_vPortSysTickHandler.c
extern void TEST_HELPER_SysTickInterrupt( void );
// from port.c
extern void vPortGetMPUregion(portSHORT regionID, xMPU_REGION_REGS *);

__attribute__((naked)) void vRTOSPendSVHandler(void) {
    __asm volatile (
        // the function below must be placed in the final line of this function, since
        // it mocks context switch between 2 tasks and never return back here.
        "b    TEST_HELPER_vPortPendSVHandler_PendSVentry \n"
        :::
    );
}

// entry function for SVC exception event
// ------------------------------------------------------------------
// when calling a sub-routine in C, 
// GCC compiler usually generates push/pop stacking instruction, 
// pushes a few GPR registers, and the LR to stack.
// the stacking behaviour of compiler might differ among differnet
// versions of toolchain . It's better to dump assembly code, 
// see how the functions are compiled.
// Here we call a routines in the inline assembly code.
// ------------------------------------------------------------------
void vRTOSSVCHandler(UBaseType_t *pulSelectedSP) {
    // get pc address from exception stack frame, then 
    // go back to find last instrution's opcode (svn <NUMBER>)
    const uint8_t recover_msp = 0xf;
    uint8_t  ucSVCnumber = ((uint8_t *) pulSelectedSP[6])[-2];
    switch(ucSVCnumber) {
        case portSVC_ID_START_SCHEDULER:
            TEST_HELPER_backupMSP();
            break;

        case portSVC_ID_RAISE_PRIVILEGE:
            TEST_HELPER_EnterCritical_SVCentry();
            break;

        case recover_msp:
            TEST_HELPER_restoreMSP();
            break;

        default : 
            break;
    };
    if(ucSVCnumber != recover_msp) {
        vPortSVCHandler(pulSelectedSP);
    }
} // end of vRTOSSVCHandler

void vRTOSSysTickHandler(void) {
    TEST_HELPER_StartScheduler_IncreSysTick();
    TEST_HELPER_EnterCritical_sysTickEntry();
    TEST_HELPER_SysTickInterrupt();
    TEST_HELPER_SuppressTickSleep_IncrSysTick();
}

BaseType_t vRTOSMemManageHandler(void) {
    return pdTRUE;
}

void vRTOSHardFaultHandler(UBaseType_t *sp) {
    TEST_HELPER_EnterCritical_hardFaultEntry(sp);
}

void vRTOSTimer3ISR(void) {}
void vRTOSTimer4ISR(void) {}

void vCopyMPUregionSetupToCheckList( xMPU_SETTINGS *xMPUSettings ) {
    // copy MPU_MBAR , MPU_RASR for the region #4 - #7
    portSHORT  idx = 0;
    for(idx=0 ; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        vPortGetMPUregion((4 + idx), &xMPUSettings->xRegion[idx]);
    }
}

#if(configUSE_TICK_HOOK > 0)
void vApplicationTickHook(void) {}
#endif

#if(configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook( TaskHandle_t pxCurrentTCBhandle, const portCHAR *pcTaskName )
{}
#endif

TEST_GROUP_RUNNER(FreeRTOS_v10_2_port) {
    RUN_TEST_CASE( pxPortInitialiseStack, initedStack_privileged );
    RUN_TEST_CASE( pxPortInitialiseStack, initedStack_unprivileged );
    RUN_TEST_CASE( TicklessSleepBareMetal , ok );
    RUN_TEST_CASE( TicklessSleepRTOS , ok );
    RUN_TEST_CASE( PortPendSVHandler, cs_without_fp );
    RUN_TEST_CASE( PortPendSVHandler, cs_with_fp );
    RUN_TEST_CASE( SysTickInterrupt, raise_ok )
    RUN_TEST_CASE( AnalogDigitalConvertor , loopback_ok );
    RUN_TEST_CASE( UARTbareMetal , loopback_ok );
    RUN_TEST_CASE( SPIbareMetal , loopback_fullduplex_dma );
    RUN_TEST_CASE( SPIbareMetal , loopback_fullduplex_blocking );
    RUN_TEST_CASE( SPIbareMetal , loopback_master2slave );
    RUN_TEST_CASE( I2CbareMetal , loopback_nonblocking_itr );
    RUN_TEST_CASE( I2CbareMetal , loopback_nonblocking_dma );
    RUN_TEST_CASE( xPortRaisePrivilege , regs_chk );
    RUN_TEST_CASE( TaskMPUSetup , with_given_region );
    RUN_TEST_CASE( TaskMPUSetup , without_given_region );
    RUN_TEST_CASE( EnterCritical , single_critical_section );
    RUN_TEST_CASE( EnterCritical , nested_critical_section );
    RUN_TEST_CASE( StartScheduler , switch2first_task );
}
