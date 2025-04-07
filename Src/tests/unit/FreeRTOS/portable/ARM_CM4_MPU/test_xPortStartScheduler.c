#include "string.h"
// for C unit test framework Unity
#include "unity_fixture.h"
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef  MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#define  TEST_STACK_SIZE  0x30

typedef struct {
    unsigned portCHAR SysTick_IP;    // systick interrupt priority
    unsigned portCHAR PendSV_IP;     // PendSV  interrupt priority
    unsigned portCHAR SVCall_IP;     // SVC exception priority
    UBaseType_t       SCB_SHCSR;     // System Handler Control & Status
    UBaseType_t       MPU_CTRL;      // MPU Control register
    UBaseType_t       SCB_CPACR;     // Coprocessor Access Control Register
    UBaseType_t       FPU_FPCCR;     // floating-point context control register
    SysTick_Type      SysTickRegs;   // all registers within SysTick 
} RegsSet_chk_t;

static StackType_t xTaskEmojiStack[TEST_STACK_SIZE] = {0};
static TaskHandle_t taskEmojiHandle = NULL;
// collecting actual data from registers & check at the end of this test
static RegsSet_chk_t  *expected_value = NULL;
static RegsSet_chk_t  *actual_value = NULL;
static volatile UBaseType_t uMockTickCount ;
static unsigned portCHAR ucVisitSVC0Flag;

// from tasks.c
extern volatile void *pxCurrentTCB;
// from port.c
extern unsigned portCHAR   ucGetMaxInNvicIPRx( void );
extern unsigned portSHORT  ucGetMaxPriGroupInAIRCR( void );

// top stack pointer, for STM32F446 platform, it is end of SRAM 0x20020000
// it may not be allowed to directly access the variable `_estack` in linker
// script since its address is 0x0 , instead the accessible address is shown
// below
static StackType_t *end_of_sram = (StackType_t *) 0x20020000;

// backup main stack pointer for recover after test
static StackType_t   msp_backup;
static StackType_t  *stackframes_backup = NULL;
static size_t   backup_nbytes = 0;

void TEST_HELPER_backupMSP(void) {
    __asm volatile (
        "mrs    %0, msp     \n"
        :"=r"(msp_backup) ::
    );
    size_t initial_sp = (size_t) end_of_sram;
    configASSERT(initial_sp > msp_backup);
    backup_nbytes = initial_sp - (size_t) msp_backup;
    stackframes_backup = (StackType_t *) unity_malloc(backup_nbytes);
    memcpy((void *)stackframes_backup, (void *)msp_backup, backup_nbytes);
    if( expected_value != NULL ) {
        ucVisitSVC0Flag = 1;
    }
}
void TEST_HELPER_restoreMSP(void) {
    __asm volatile (
        "mov r0 ,  #0 \n"
        "msr control,  r0 \n"
        // hacky, replace the 2nd stack frame (which should be link
        // register of `TEST_HELPER_backupMSP`) with current link register
        // , in order to return from start-scheduler function.
        "mov %0 ,  lr \n"
        :"=r"(stackframes_backup[1]) ::
    );
    memcpy((void *)msp_backup, (void *)stackframes_backup, backup_nbytes);
    __asm volatile (
        "msr msp,  %0 \n" // switch back to previous msp in main thread
        :: "r"(msp_backup)
    );
}
void TEST_HELPER_StartScheduler_IncreSysTick(void) {
    if( expected_value != NULL ) {
        uMockTickCount++;
    }
}
static void vTaskFunction_emoji(void *pvParameters) {
    // SP_prccess is used in all tasks, shouldn't affect SP_main
    TaskStatus_t taskInfo = {0};
    vTaskGetInfo(taskEmojiHandle, &taskInfo, pdFALSE, eInvalid);
    configASSERT(taskInfo.eCurrentState == eRunning);
    __asm volatile(
        "svc  #0x0f" // switch back to main thread task
    ); 
    configASSERT(pdFALSE);
}

TEST_GROUP( StartScheduler );

TEST_SETUP( StartScheduler ) {
    uMockTickCount = 0;
    ucVisitSVC0Flag = 0;
    expected_value = (RegsSet_chk_t *) unity_malloc( sizeof(RegsSet_chk_t) );
    actual_value   = (RegsSet_chk_t *) unity_malloc( sizeof(RegsSet_chk_t) );
    const TaskParameters_t tsk_params = {
        .pvTaskCode = vTaskFunction_emoji,
        .pcName = "E-Mo-Ji",
        .usStackDepth = TEST_STACK_SIZE,
        .pvParameters = NULL,
        .uxPriority = portPRIVILEGE_BIT | (tskIDLE_PRIORITY + 1),
        .puxStackBuffer = xTaskEmojiStack,
    };
    BaseType_t ret = xTaskCreateRestricted(&tsk_params, &taskEmojiHandle);
    configASSERT(ret == pdPASS);
}

TEST_TEAR_DOWN( StartScheduler )
{
    // disable interrupt for SVC, SysTick, and Memory Management Fault
    NVIC_SetPriority( SysTick_IRQn, 0 );
    NVIC_SetPriority( PendSV_IRQn , 0 );
    NVIC_SetPriority( SVCall_IRQn , 0 );
    __asm volatile(
        "cpsid i  \n" 
        "cpsid f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
    // turn off the features we set for the test
    SCB->SHCSR  &= ~SCB_SHCSR_MEMFAULTENA_Msk;
    MPU->CTRL   &= ~(MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk);
    FPU->FPCCR  &= ~FPU_FPCCR_LSPEN_Msk;
    SysTick->CTRL  = 0;
    SysTick->VAL   = 0;
    SysTick->LOAD  = 0;
    // free memory allocated to check lists 
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    if(stackframes_backup) {
        unity_free((void *) stackframes_backup);
        stackframes_backup = NULL;
        backup_nbytes = 0;
    }
    expected_value = NULL; 
    actual_value   = NULL; 
    vTaskDelete(taskEmojiHandle);
    taskEmojiHandle = NULL;
    pxCurrentTCB   = NULL;
} // end of TEST_TEAR_DOWN

TEST(StartScheduler, switch2first_task)
{
    UBaseType_t  uTickCountUpperBound = 0;

    xPortStartScheduler();
    TEST_ASSERT_NOT_EQUAL( configMAX_SYSCALL_INTERRUPT_PRIORITY , 0 );
    // check priority of SysTick, PendSV exception
    expected_value->SysTick_IP = (unsigned portCHAR) configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    actual_value->SysTick_IP   = (unsigned portCHAR) NVIC_GetPriority( SysTick_IRQn );
    TEST_ASSERT_EQUAL_UINT8( expected_value->SysTick_IP, actual_value->SysTick_IP );
    expected_value->PendSV_IP  = (unsigned portCHAR) configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    actual_value->PendSV_IP    = (unsigned portCHAR) NVIC_GetPriority( PendSV_IRQn );
    TEST_ASSERT_EQUAL_UINT8( expected_value->PendSV_IP, actual_value->PendSV_IP);
    expected_value->SVCall_IP = (unsigned portCHAR) configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY ;
    actual_value->SVCall_IP   = (unsigned portCHAR) NVIC_GetPriority( SVCall_IRQn );
    TEST_ASSERT_EQUAL_UINT8( expected_value->SVCall_IP, actual_value->SVCall_IP );     

    // check other system registers ....
    expected_value->SCB_SHCSR = SCB_SHCSR_MEMFAULTENA_Msk; 
    actual_value->SCB_SHCSR   = SCB->SHCSR & SCB_SHCSR_MEMFAULTENA_Msk; 
    expected_value->MPU_CTRL  = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk; 
    actual_value->MPU_CTRL    = MPU->CTRL ; 
    expected_value->SCB_CPACR = SCB_CPACR_CP11_Msk | SCB_CPACR_CP10_Msk; //  configure to fully access CP10 & CP11
    actual_value->SCB_CPACR   = SCB->CPACR;
    expected_value->FPU_FPCCR = FPU_FPCCR_LSPEN_Msk;
    actual_value->FPU_FPCCR   = FPU->FPCCR & FPU_FPCCR_LSPEN_Msk;
    expected_value->SysTickRegs.CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |  SysTick_CTRL_ENABLE_Msk ;
    actual_value->SysTickRegs.CTRL   = SysTick->CTRL;
    expected_value->SysTickRegs.LOAD = (configCPU_CLOCK_HZ / configTICK_RATE_HZ)- 1UL;
    actual_value->SysTickRegs.LOAD   = SysTick->LOAD;
    expected_value->SysTickRegs.VAL  = 0UL;
    TEST_ASSERT_EQUAL_UINT32( expected_value->SCB_SHCSR, actual_value->SCB_SHCSR );
    TEST_ASSERT_EQUAL_UINT32( expected_value->MPU_CTRL , actual_value->MPU_CTRL  );
    TEST_ASSERT_EQUAL_UINT32( expected_value->SCB_CPACR, actual_value->SCB_CPACR );
    TEST_ASSERT_EQUAL_UINT32( expected_value->FPU_FPCCR, actual_value->FPU_FPCCR );
    TEST_ASSERT_EQUAL_UINT32( expected_value->SysTickRegs.CTRL , actual_value->SysTickRegs.CTRL );
    TEST_ASSERT_EQUAL_UINT32( expected_value->SysTickRegs.LOAD , actual_value->SysTickRegs.LOAD );
    uTickCountUpperBound = uMockTickCount + 10;
    while (uMockTickCount < uTickCountUpperBound) {
        actual_value->SysTickRegs.VAL = SysTick->VAL;
        TEST_ASSERT_UINT32_WITHIN( ((expected_value->SysTickRegs.LOAD + 1) >> 1),
                                   ((expected_value->SysTickRegs.LOAD + 1) >> 1),
                                   actual_value->SysTickRegs.VAL );
    }
    TEST_ASSERT_EQUAL_UINT8( 1, ucVisitSVC0Flag );
} //// end of test body

