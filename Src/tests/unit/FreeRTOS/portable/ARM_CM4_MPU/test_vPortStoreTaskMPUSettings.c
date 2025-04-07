// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

#define  MOCK_STACK_DEPTH  0x30

extern void vPortGetMPUregion(portSHORT, xMPU_REGION_REGS *);
extern void vPortSetMPUregion(xMPU_REGION_REGS *);

static xMPU_SETTINGS  *expected_value = NULL;
static xMPU_SETTINGS  *actual_value   = NULL;
static UBaseType_t    *ulOptionalVariable[5] ;
static StackType_t    *ulMockStackMemory = NULL;

TEST_GROUP( TaskMPUSetup );

TEST_SETUP( TaskMPUSetup ) {
    portSHORT idx = 0;
    expected_value = (xMPU_SETTINGS *) unity_malloc( sizeof(xMPU_SETTINGS) );
    actual_value   = (xMPU_SETTINGS *) unity_malloc( sizeof(xMPU_SETTINGS) );
    for(idx=0; idx<5; idx++)
    {
        ulOptionalVariable[idx]    = (UBaseType_t *) unity_malloc( sizeof(UBaseType_t) );
        *(ulOptionalVariable[idx]) = ((idx+1) << 24) | ((idx+1) << 16) | ((idx+1) << 8) | (idx+1);
    }
    ulMockStackMemory = (StackType_t *) unity_malloc( sizeof(StackType_t) * MOCK_STACK_DEPTH );
}

TEST_TEAR_DOWN( TaskMPUSetup ) {
    portSHORT idx = 0;
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
    for(idx=0; idx<5; idx++) {
        unity_free( (void *)ulOptionalVariable[idx] ); 
        ulOptionalVariable[idx] = NULL;
    }
    unity_free( (void *)ulMockStackMemory ); 
    ulMockStackMemory = NULL; 
    // clear the settings of extra regions for this test 
    xMPU_REGION_REGS  xRegion;
    xRegion.RBAR = MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION  ;
    xRegion.RASR = 0;
    vPortSetMPUregion( &xRegion );
    xRegion.RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION+1)  ;
    vPortSetMPUregion( &xRegion );
}


// user applications specify regions to task TCB
TEST( TaskMPUSetup , with_given_region )
{
    portSHORT       idx;
    UBaseType_t     ulRegionSizeInBytes ;
    MemoryRegion_t  xMemRegion[ portNUM_CONFIGURABLE_REGIONS + 1 ];

    xMemRegion[0].pvBaseAddress   = (void *)ulOptionalVariable[0];
    xMemRegion[0].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[0].ulParameters    =  portMPU_REGION_PRIV_RW_URO | MPU_RASR_S_Msk ;
    xMemRegion[1].pvBaseAddress   = (void *)ulOptionalVariable[2] ;
    xMemRegion[1].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[1].ulParameters    =  portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_C_Msk | MPU_RASR_B_Msk;
    xMemRegion[2].pvBaseAddress   = (void *)ulOptionalVariable[4] ;
    xMemRegion[2].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[2].ulParameters    =  portMPU_REGION_READ_ONLY | MPU_RASR_S_Msk | MPU_RASR_B_Msk;
    // stack memory is supposed to be fully accessed by a task.
    ulRegionSizeInBytes = (UBaseType_t)sizeof(StackType_t) * MOCK_STACK_DEPTH;
    expected_value->xRegion[0].RBAR  = ((UBaseType_t)ulMockStackMemory & MPU_RBAR_ADDR_Msk) | 
                                       MPU_RBAR_VALID_Msk | portSTACK_REGION  ;
    expected_value->xRegion[0].RASR  = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;
    // xRegion[1] - xRegion[3] : optional variable used for this mock task
    ulRegionSizeInBytes = (UBaseType_t)sizeof(UBaseType_t) ;
    expected_value->xRegion[1].RBAR  = ((UBaseType_t)ulOptionalVariable[0] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION    ;
    expected_value->xRegion[1].RASR  = portMPU_REGION_PRIV_RW_URO | MPU_RASR_S_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    expected_value->xRegion[2].RBAR  = ((UBaseType_t)ulOptionalVariable[2] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    expected_value->xRegion[2].RASR  = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    expected_value->xRegion[3].RBAR  = ((UBaseType_t)ulOptionalVariable[4] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
    expected_value->xRegion[3].RASR  = portMPU_REGION_READ_ONLY | MPU_RASR_S_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    vPortStoreTaskMPUSettings(actual_value, xMemRegion, ulMockStackMemory, MOCK_STACK_DEPTH);
    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1) ; idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RBAR , actual_value->xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RASR , actual_value->xRegion[idx].RASR );
    }

    // ------------- modify one of the MPU region's settings then verify again. -------------
    xMemRegion[0].pvBaseAddress   = (void *)ulOptionalVariable[3];
    xMemRegion[0].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[0].ulParameters    =  portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk ;
    xMemRegion[1].pvBaseAddress   = (void *)NULL ;
    xMemRegion[1].ulLengthInBytes = 0x0 ;
    xMemRegion[1].ulParameters    = portMPU_REGION_READ_ONLY | MPU_RASR_C_Msk | MPU_RASR_B_Msk;
    ulRegionSizeInBytes = (UBaseType_t)sizeof(UBaseType_t) ;
    expected_value->xRegion[1].RBAR  = ((UBaseType_t)ulOptionalVariable[3] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION    ;
    expected_value->xRegion[1].RASR  = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    expected_value->xRegion[2].RBAR  = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    expected_value->xRegion[2].RASR  = 0x0 ;

    vPortStoreTaskMPUSettings(actual_value, xMemRegion, ulMockStackMemory, MOCK_STACK_DEPTH);

    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1) ; idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RBAR , actual_value->xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RASR , actual_value->xRegion[idx].RASR );
    }
} //// end of test body


TEST( TaskMPUSetup , without_given_region )
{
    portSHORT  idx;
    UBaseType_t     ulRegionSizeInBytes ;
    extern  UBaseType_t  __SRAM_segment_start__[] ;
    extern  UBaseType_t  __SRAM_segment_end__  [] ;
    extern  UBaseType_t  __privileged_data_start__[];
    extern  UBaseType_t  __privileged_data_end__[];

    ulRegionSizeInBytes = (UBaseType_t)__SRAM_segment_end__    - (UBaseType_t)__SRAM_segment_start__ ;
    expected_value->xRegion[0].RBAR = ((UBaseType_t)__SRAM_segment_start__ & MPU_RBAR_ADDR_Msk)
                                    | MPU_RBAR_VALID_Msk | portSTACK_REGION;
    expected_value->xRegion[0].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                    | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                    |  MPU_RASR_ENABLE_Msk ;
    ulRegionSizeInBytes = (UBaseType_t)__privileged_data_end__ - (UBaseType_t)__privileged_data_start__ ;
    expected_value->xRegion[1].RBAR = ((UBaseType_t)__privileged_data_start__ & MPU_RBAR_ADDR_Msk)
                                     | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION;
    expected_value->xRegion[1].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                    | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                    |  MPU_RASR_ENABLE_Msk ;

    expected_value->xRegion[2].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    expected_value->xRegion[2].RASR = 0x0;
    expected_value->xRegion[3].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
    expected_value->xRegion[3].RASR = 0x0;

    vPortStoreTaskMPUSettings(actual_value, NULL, NULL, 0);

    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1) ; idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RBAR , actual_value->xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RASR , actual_value->xRegion[idx].RASR );
    }
} //// end of test body





// ---------------- TODO, WIP -----------------------

PRIVILEGED_FUNCTION static void prvUpdateMPUcheckList (xMPU_SETTINGS *target)
{
    extern  UBaseType_t  __privileged_code_end__[];
    extern  UBaseType_t  __code_segment_start__ [];
    extern  UBaseType_t  __code_segment_end__[];
    extern  UBaseType_t  __privileged_data_start__[];
    extern  UBaseType_t  __privileged_data_end__[];
    UBaseType_t  ulRegionSizeInBytes = 0;

    // Note: MPU_RBAR.VALID is always read as zero, no need to check this bit.
    // unprivileged code section (all FLASH memory)
    ulRegionSizeInBytes = (UBaseType_t)__code_segment_end__ - (UBaseType_t)__code_segment_start__;
    target->xRegion[0].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk) | portUNPRIVILEGED_FLASH_REGION;
    target->xRegion[0].RASR = portMPU_REGION_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                              ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                              | MPU_RASR_ENABLE_Msk ;

    // privileged code section (first few KBytes of the FLASH memory, determined by application)
    ulRegionSizeInBytes = (UBaseType_t)__privileged_code_end__  - (UBaseType_t)__code_segment_start__;
    target->xRegion[1].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk) | portPRIVILEGED_FLASH_REGION ;
    target->xRegion[1].RASR = portMPU_REGION_PRIVILEGED_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                              ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                              | MPU_RASR_ENABLE_Msk ;

    // privileged data section (first few KBytes of the SRAM memory, determined by application)
    ulRegionSizeInBytes = (UBaseType_t) __privileged_data_end__ - (UBaseType_t)__privileged_data_start__;
    target->xRegion[2].RBAR = ((UBaseType_t) __privileged_data_start__ & MPU_RBAR_ADDR_Msk) | portPRIVILEGED_SRAM_REGION;
    target->xRegion[2].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                             ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                             | MPU_RASR_ENABLE_Msk ;

    // peripheral region should have different attributes from normal memory
    target->xRegion[3].RBAR = ((UBaseType_t) PERIPH_BASE & MPU_RBAR_ADDR_Msk) | portGENERAL_PERIPHERALS_REGION ;
    target->xRegion[3].RASR = (portMPU_REGION_READ_WRITE | portMPU_REGION_EXEC_NEVER) | MPU_RASR_ENABLE_Msk |
                              ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(PERIPH_SIZE) << MPU_RASR_SIZE_Pos) );
} //// end of prvUpdateMPUcheckList


// for testing purpose, we copy attributes from region #2 to region #6
// , and add region #5 to allow both privileged/unprivileged accesses,
// * 
static void vModifyMPUregionsForTest( void )
{
    extern  UBaseType_t  __SRAM_segment_start__[];
    extern  UBaseType_t  __SRAM_segment_end__ [];
    xMPU_REGION_REGS  xRegion;
    UBaseType_t  ulRegionSizeInBytes = 0;
    // move attributes from region #2 to region #6
    vPortGetMPUregion( portPRIVILEGED_SRAM_REGION, &xRegion );
    xRegion.RBAR &= ~(MPU_RBAR_REGION_Msk);
    xRegion.RBAR |= MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    vPortSetMPUregion( &xRegion );
    // setup one more MPU region only for unprivileged accesses in this test to SRAM
    ulRegionSizeInBytes = (UBaseType_t) __SRAM_segment_end__ - (UBaseType_t) __SRAM_segment_start__ ;
    xRegion.RBAR = ((UBaseType_t) __SRAM_segment_start__  & MPU_RBAR_ADDR_Msk)
                      | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION  ;
    xRegion.RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                     ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                     | MPU_RASR_ENABLE_Msk;
    vPortSetMPUregion( &xRegion );
} //// end of vModifyMPUregionsForTest


PRIVILEGED_DATA static volatile UBaseType_t uMockPrivVar ;
static unsigned portCHAR  unpriv_branch_fail_cnt = 0;

TEST( TaskMPUSetup , unpriv_trig_excpt ) {
    xMPU_SETTINGS expect = {0}, actual = {0};  // record settings of first 4 MPU regions (#0 - #3)
    uMockPrivVar  = 0xdead;
    vModifyMPUregionsForTest();
    // part 1: 
    // CPU in Thread mode switches to unprivileged state, then calls the privileged function,
    // this invalid call should result in HardFault exception.
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x1 \n"
        "msr  control, r8   \n"
        "dsb  \n"
        "isb  \n"
    );
    prvUpdateMPUcheckList(&expect);
    // when CPU jumps back here, it becomes privileged thread mode
    // (switch the state in our HardFault handler routine)
    TEST_ASSERT_EQUAL_UINT16( unpriv_branch_fail_cnt , 1 );
    
    // part 2: 
    // CPU switches back to privileged state, then invoke privileged function again
    prvUpdateMPUcheckList(&expect);
     // the counter should still remain the same
    TEST_ASSERT_EQUAL_UINT16( unpriv_branch_fail_cnt , 1 );
    TEST_ASSERT_NOT_EQUAL( unpriv_branch_fail_cnt , 2 );

    // part 3: 
    // CPU in Thread mode switches to unprivileged state again, then accesses a privileged variable,
    // this invalid access should result in HardFault exception.
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x1 \n"
        "msr  control, r8   \n"
        "dsb  \n"
        "isb  \n"
    );
    // following line of code will be executed twice, first time it runs at unprivileged thread mode,
    // , then jump to HardFault exception handler routine, which switches back to privileged mode,
    // then jump back here, the same line of code, and execute again at privileged thread mode.
    uMockPrivVar   = 0xacce55ed;
    TEST_ASSERT_EQUAL_UINT32( uMockPrivVar, 0xacce55ed);
    TEST_ASSERT_EQUAL_UINT16( unpriv_branch_fail_cnt , 2 );

    // check the MPU regions settings 
    for(int idx=0 ; idx<4 ; idx++) {
        vPortGetMPUregion(idx, &actual.xRegion[idx] );
        TEST_ASSERT_EQUAL_UINT32( expect.xRegion[idx].RBAR , actual.xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expect.xRegion[idx].RASR , actual.xRegion[idx].RASR );
    }
} // end of test body
// ---------------- TODO, WIP -----------------------

