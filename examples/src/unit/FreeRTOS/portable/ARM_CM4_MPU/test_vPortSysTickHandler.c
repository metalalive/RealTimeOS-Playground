#include "unity_fixture.h"
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

typedef struct {
    UBaseType_t  sysTickFlag;
} flagSet_chk_t;

static flagSet_chk_t  *actual_value   = NULL;


void TEST_HELPER_SysTickInterrupt(void) {
    if(actual_value != NULL) {
        uint32_t cnt_flg = SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk;
        configASSERT( cnt_flg == SysTick_CTRL_COUNTFLAG_Msk );
        SysTick->CTRL = 0L;
        SysTick->VAL = 0;
        actual_value->sysTickFlag = 1;
    }
}

TEST_GROUP(SysTickInterrupt);

TEST_SETUP(SysTickInterrupt) {
    actual_value   = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    actual_value->sysTickFlag = 0;
    // enable SysTick timer
    SysTick->CTRL = 0L;
    SysTick->VAL  = 0L;
    SysTick->LOAD = (configCPU_CLOCK_HZ / (configTICK_RATE_HZ*20))- 1UL;
    // enable interrupt & set proper priority
    NVIC_SetPriority( SysTick_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    __asm volatile(
        "cpsie i  \n" 
        "cpsie f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
}

TEST_TEAR_DOWN( SysTickInterrupt ) {
    // free memory allocated to check lists 
    unity_free( (void *)actual_value   ); 
    actual_value   = NULL; 
    // disable interrupt & reset priority
    __asm volatile(
        "dsb      \n" 
        "isb      \n"
        "cpsid i  \n" 
        "cpsid f  \n" 
        :::"memory"
    );
    SysTick->CTRL  = 0;
    SysTick->VAL   = 0;
    SysTick->LOAD  = 0;
    NVIC_SetPriority( SysTick_IRQn, 0 );
}

TEST(SysTickInterrupt, raise_ok) {
    for(size_t idx = 0; idx < 5; idx++) {
        actual_value->sysTickFlag = 0;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk ;
        while (actual_value->sysTickFlag == 0);
        TEST_ASSERT_EQUAL_UINT32( 0, SysTick->VAL );
        TEST_ASSERT_EQUAL_UINT32( 0, SysTick->CTRL );
        TEST_ASSERT_EQUAL_UINT32( 1, actual_value->sysTickFlag );
    }
}
