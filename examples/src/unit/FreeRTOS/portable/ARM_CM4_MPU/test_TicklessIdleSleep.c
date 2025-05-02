// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

extern TIM_HandleTypeDef htim3;
static UBaseType_t *expected_value = NULL;
static UBaseType_t *actual_value   = NULL;

void TEST_HELPER_SuppressTickSleep_IncrSysTick(void) {
}

void MX_TIM3_Init(void);

TEST_GROUP( TicklessSleepRTOS );
TEST_GROUP( TicklessSleepBareMetal );

// FIXME , WIP, in this test case , SysTick interrupt NEVER wakes ARM CPU even
// there is no other pending interrupt with higher priority.
// TODO , confirm hardware issue.
// Currently this test case is skipped until I can fix this strange issue.
TEST_SETUP( TicklessSleepRTOS ) {
    __set_BASEPRI(0x0); // unmask all interrupts 
    expected_value  = (UBaseType_t *) unity_malloc( sizeof(UBaseType_t) );
    actual_value    = (UBaseType_t *) unity_malloc( sizeof(UBaseType_t) );
    *expected_value = 0;
    *actual_value   = 0;
    SCB->SHCSR |= SCB_SHCSR_SYSTICKACT_Msk;
    NVIC_SetPriority( SysTick_IRQn, 0x1 );    
    configASSERT(SysTick_CTRL_CLKSOURCE_Msk == SYSTICK_CLKSOURCE_HCLK);
#if 0 // experiment with only WFI
    SysTick->VAL  = 0xa1b0;
    SysTick->LOAD = 0xa1b2;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
    __asm volatile(
        "cpsie  i \n" 
        "dsb      \n" 
        "wfi      \n" 
        "isb      \n"
        :::"memory"
    );
#endif
}

TEST_TEAR_DOWN(TicklessSleepRTOS) {
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
    SysTick->CTRL = 0;
    SysTick->VAL  = 0;
    SysTick->LOAD = 0;
    NVIC_SetPriority( SysTick_IRQn, 0 );
    __set_BASEPRI(0x0);
    SCB->SHCSR &= ~SCB_SHCSR_SYSTICKACT_Msk;
}


// add preprocessor condition  configUSE_TICKLESS_IDLE == 1 once
// the issue is fixed

IGNORE_TEST( TicklessSleepRTOS, ok ) {
    TickType_t  xTicksToSleep;
    TickType_t  ulTimerCountsPerTick = SysTick->LOAD ;
    portSHORT   idx = 0, jdx = 0;
    TickType_t  xMockTickCount = 0;

    for (idx=0; idx<20; idx++) {
        xMockTickCount = xTaskGetTickCount();
        
        for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
        xTicksToSleep = 4;
        *expected_value = xMockTickCount + xTicksToSleep;
        vPortSuppressTicksAndSleep( xTicksToSleep );
        // At here, we check number of ticks passed during the low-power entry function
        *actual_value = xTaskGetTickCount();
        TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );

        for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
        xTicksToSleep = 5;
        *expected_value = xMockTickCount + xTicksToSleep;
        vPortSuppressTicksAndSleep( xTicksToSleep );
        *actual_value = xTaskGetTickCount();
        TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );

        for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
        xTicksToSleep = 300;
        *expected_value = xMockTickCount + xTicksToSleep;
        vPortSuppressTicksAndSleep( xTicksToSleep );
        *actual_value = xTaskGetTickCount();
        TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );
    } // end of loop
} // end of test body

TEST_SETUP( TicklessSleepBareMetal ) {
    __asm volatile(
        "cpsid i  \n" // disable CPU to enter ISR after wakeup from wfi
        "isb      \n"
        :::"memory"
    );
    __HAL_DBGMCU_FREEZE_TIM3();
    MX_TIM3_Init();
}
TEST_TEAR_DOWN(TicklessSleepBareMetal) {
    __HAL_DBGMCU_UNFREEZE_TIM3();
    __asm volatile(
        "cpsie i  \n" 
        "isb      \n"
        :::"memory"
    );
    HAL_StatusTypeDef result = HAL_TIM_Base_Stop_IT(&htim3);
    configASSERT(HAL_OK == result);
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    NVIC_DisableIRQ(TIM3_IRQn);
    NVIC_ClearPendingIRQ(TIM3_IRQn);
}

TEST( TicklessSleepBareMetal , ok ) {
    // clear needless pending interrupt on timer initialization
    for(int idx = 1; idx < 5; idx++) {
        HAL_StatusTypeDef result = HAL_TIM_Base_Stop_IT(&htim3);
        configASSERT(HAL_OK == result);
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
        NVIC_DisableIRQ(TIM3_IRQn);
        NVIC_ClearPendingIRQ(TIM3_IRQn);
        NVIC_EnableIRQ(TIM3_IRQn);
        result = HAL_TIM_Base_Start_IT(&htim3);
        configASSERT(HAL_OK == result);
        __asm volatile(
            "dsb      \n" 
            "wfi      \n" 
            "isb      \n"
            :::"memory"
        );
        // TIM3 SR.UiF bit should not be reset in TIM3_IRQHandler 
        uint32_t interrupt_raised = __HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE);
        configASSERT(interrupt_raised == 1);
    }
} // end of test body
