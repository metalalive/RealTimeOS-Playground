#include "test_runner.h"

void vCreateAllTestTasks(void) {
    vSetupNestedInterruptTest();
    vStartIntegerMathTasks( tskIDLE_PRIORITY );
    vStartDynamicPriorityCase2( tskIDLE_PRIORITY );
    vStartDynamicPriorityCase1( tskIDLE_PRIORITY );
    #if (configCHECK_FOR_STACK_OVERFLOW > 0)
        vStartStackOverflowCheck( tskIDLE_PRIORITY );
    #endif
    vStartBlockTimeTasks( configMAX_PRIORITIES - 4 );
    vStartQueueTestCase1( tskIDLE_PRIORITY );
    vStartQueueTestCase2( tskIDLE_PRIORITY );
    vStartQueueTestCase3( tskIDLE_PRIORITY + 1 );
    vStartBinSemphrCase1( tskIDLE_PRIORITY );
    vStartBinSemphrCase2( configMAX_PRIORITIES - 4 );

    #if (configUSE_COUNTING_SEMAPHORES == 1)
        vStartCountSemphrTest( tskIDLE_PRIORITY );
    #endif
    #if (configUSE_MUTEXES == 1)
        vStartMutexTestCase1( tskIDLE_PRIORITY );
        #if (configUSE_RECURSIVE_MUTEXES == 1)
            vStartRecurMutexTest( tskIDLE_PRIORITY );
        #endif // end of configUSE_RECURSIVE_MUTEXES
    #endif 
    #if (configUSE_TASK_NOTIFICATIONS == 1)
        vStartNotifyTaskTest( tskIDLE_PRIORITY );
    #endif
    #if (configUSE_TIMERS == 1)
        vStartSoftwareTimerTest( tskIDLE_PRIORITY );
    #endif
} // end of vCreateAllTestTasks


void vRTOSTimer3ISR(void) {
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vNestInterruptTestISR1();
        vQueueTestCase3ISR1();
        vBinSemphrCase2ISR1();
    }
}
void vRTOSTimer4ISR(void) {
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vNestInterruptTestISR2();
        vQueueTestCase3ISR2();
        #if (configUSE_TASK_NOTIFICATIONS == 1)
            vNotifyTestISR( );
        #endif //// end of configUSE_TASK_NOTIFICATIONS
    }
}
BaseType_t vRTOSMemManageHandler(void) {
    BaseType_t alreadyHandled = pdFALSE;
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        #if (configCHECK_FOR_STACK_OVERFLOW > 0)
            alreadyHandled = vStackOvflFaultHandler();
        #endif
    }
    return alreadyHandled;
}
void vRTOSHardFaultHandler(UBaseType_t *sp) {
    BaseType_t done = vPortTryRecoverHardFault(sp);
    if(!done) {
        while(1);
    }
}
void vRTOSSVCHandler(UBaseType_t *sp) {
    vPortSVCHandler(sp);
}
void vRTOSSysTickHandler(void) {
    vPortSysTickHandler();
}
__attribute__((naked)) void vRTOSPendSVHandler(void) {
    // vPortPendSVHandler(); the function call in C will be compiled with
    // branch-link instruction, which may corrupts link register of current task.
    __asm volatile ("b vPortPendSVHandler  \n");
}

#if (configUSE_TICK_HOOK > 0)
void vApplicationTickHook( void )
{
    vNestInterruptTestTickHook();
}
#endif // end of configUSE_TICK_HOOK
