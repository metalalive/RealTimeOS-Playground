#include "tests/integration/FreeRTOS/semphr_bin_case2.h"

// in our case, we create tasks' stack using privileged function  pvPortMalloc() 
// implemented in heap_4.c, each allocated block memory preserves first 8 bytes 
// for  internal data structure, representing size of the allocated block.
#define  intgSTACK_SIZE      (( unsigned portSHORT ) 0x3e)
#define  BSEM_SHR_VAR_TSK_FLAG        0x71
#define  BSEM_SHR_VAR_ISR_FLAG        0x8e

// the data structure below is used only for passing parameters to the tasks in this tests
typedef struct {
    // the queue used by the tasks
    SemaphoreHandle_t     xSemphr; 
    // the blocking time can be applied to Queue send/receive operations, or task delay functions
    TickType_t            xBlockTime;
    // shared variables used in this test
    volatile UBaseType_t *pulSharedVariable;
} smphrParamStruct;

// decalre parameter structure that can be accessed by interrupt and  task
static volatile smphrParamStruct *semParams = NULL;


void vBinSemphrCase2ISR1(void) {
    SemaphoreHandle_t     xSemphr = semParams->xSemphr ; 
    volatile UBaseType_t *pulSharedVariable = semParams->pulSharedVariable ;
    BaseType_t  pxHPtaskWoken = pdFALSE;
    if ((*pulSharedVariable) != BSEM_SHR_VAR_ISR_FLAG) {
        TEST_ASSERT_EQUAL_UINT32(BSEM_SHR_VAR_TSK_FLAG, (*pulSharedVariable));
        *pulSharedVariable = BSEM_SHR_VAR_ISR_FLAG;
        xSemaphoreGiveFromISR( xSemphr, &pxHPtaskWoken );
        portYIELD_FROM_ISR(pxHPtaskWoken);
    }
}

static void vBinSmphrKs2Taker(void *pvParams) {
    smphrParamStruct *lclsemParams = (smphrParamStruct *) pvParams;
    SemaphoreHandle_t     xSemphr             = lclsemParams->xSemphr ; 
    TickType_t            xBlockTime          = lclsemParams->xBlockTime ;
    volatile UBaseType_t *pulSharedVariable   = lclsemParams->pulSharedVariable ;
    for(;;) {
        BaseType_t semOpsStatus = xSemaphoreTake(xSemphr, xBlockTime);
        if(semOpsStatus == pdPASS) {
            TEST_ASSERT_EQUAL_UINT32(BSEM_SHR_VAR_ISR_FLAG, (*pulSharedVariable));
            *pulSharedVariable = BSEM_SHR_VAR_TSK_FLAG;
        }
    }
}

void vStartBinSemphrCase2( UBaseType_t uxPriority )
{
    StackType_t        *stackMemSpace ;
    BaseType_t          xState; 
    unsigned portSHORT  idx;

    stackMemSpace = (StackType_t *) pvPortMalloc( sizeof(StackType_t) * intgSTACK_SIZE );
    // initialize the parameter structure
    semParams = (smphrParamStruct *) pvPortMalloc( sizeof(smphrParamStruct) );
    configASSERT( semParams );
    semParams->xSemphr = xSemaphoreCreateBinary();
    configASSERT( semParams->xSemphr );
    semParams->pulSharedVariable = (UBaseType_t *) pvPortMalloc( sizeof(UBaseType_t) );
    configASSERT( semParams->pulSharedVariable );
    *(semParams->pulSharedVariable) = BSEM_SHR_VAR_TSK_FLAG;
    semParams->xBlockTime  = portMAX_DELAY;
    
    TaskParameters_t tskparams = {
        vBinSmphrKs2Taker, "BSemKs2Taker", intgSTACK_SIZE, (void *)semParams,
        (uxPriority | portPRIVILEGE_BIT), stackMemSpace,
        // leave MPU regions uninitialized
    };
    // default value to unused MPU regions 
    for(idx=0; idx<portNUM_CONFIGURABLE_REGIONS; idx++)
    {
        tskparams.xRegions[idx].pvBaseAddress   = NULL;
        tskparams.xRegions[idx].ulLengthInBytes = 0;
        tskparams.xRegions[idx].ulParameters    = 0;
    }
    xState = xTaskCreateRestricted( (const TaskParameters_t * const)&tskparams, NULL );
    configASSERT( xState == pdPASS );
    // xTaskCreate( vBinSmphrKs2Taker, "BSemKs2Taker", intgSTACK_SIZE, (void *)semParams, uxPriority, NULL);
} // end of vStartBinSemphrCase2()
