#ifndef __INTEGRATION_RTOS_TEST_RUNNER_H
#define __INTEGRATION_RTOS_TEST_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef  MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "nestISR.h"
#include "integer.h"
#include "dynamic_case1.h"
#include "dynamic_case2.h"
#include "block_time.h"
#include "queue_case1.h"
#include "queue_case2.h"
#include "queue_case3.h"
#include "semphr_bin_case1.h"
#include "semphr_bin_case2.h"

#if (configUSE_COUNTING_SEMAPHORES == 1)
    #include "semphr_cnt.h"
#endif

#if (configUSE_MUTEXES == 1)
    #include "mutex_case1.h"
    #if (configUSE_RECURSIVE_MUTEXES == 1)
        #include "recur_mutex.h"
    #endif
#endif 

#if (configUSE_TASK_NOTIFICATIONS == 1)
    #include "notify.h"
#endif

#if (configUSE_TIMERS == 1)
    #include "sw_timer.h"
#endif

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
    #include "stack_ovfl_chk.h"
#endif

// ----------- function declaration -----------
void vCreateAllTestTasks( void );

// there are few integration tests that use tick hook function,
// therefore configUSE_TICK_HOOK should be set
#if (configUSE_TICK_HOOK > 0)
void vApplicationTickHook( void );
#endif // end of configUSE_TICK_HOOK

#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_RTOS_TEST_RUNNER_H
