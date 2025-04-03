#include "tests/integration/FreeRTOS/queue_case1.h"

#define  NUM_OF_TASKS        6
// in our case, we create tasks' stack using privileged function  pvPortMalloc() 
// implemented in heap_4.c, each allocated block memory preserves first 8 bytes 
// for  internal data structure, representing size of the allocated block.
#define  intgSTACK_SIZE       ( ( unsigned portSHORT ) 0x3e )
#define  PROCESS2STR(TOKEN)  #TOKEN

// the data structure below is used only for passing parameters to the tasks in this tests
typedef struct {
    // the queue used by the tasks
    QueueHandle_t        xQueue;    
    // the blocking time applied in Queue send/receive operations
    TickType_t           xBlockTime;
} qCaseParamStruct;

static void vQTestKs1Producer(void *pvParams) {
    qCaseParamStruct *qParams = (qCaseParamStruct *) pvParams;
    QueueHandle_t     xQueue  =  qParams->xQueue;
    TickType_t        xBlockTime = qParams->xBlockTime;
    BaseType_t QopsStatus = pdPASS;
    unsigned portSHORT qItem = 0;
    for(;;) {
        // for producer, the send operation must be done with pdPASS in the 3 planned scenarios
        qItem++ ;
        QopsStatus = errQUEUE_FULL;
        QopsStatus = xQueueSendToBack( xQueue, (void *)&qItem, xBlockTime );
        TEST_ASSERT_EQUAL_INT32(pdPASS, QopsStatus);
    }
} // end of vQTestKs1Producer

static void vQTestKs1Consumer(void *pvParams) {
    qCaseParamStruct *qParams = (qCaseParamStruct *) pvParams;
    QueueHandle_t     xQueue = qParams->xQueue;
    TickType_t    xBlockTime = qParams->xBlockTime;
    unsigned portSHORT qItemActual = 0, qItemExpected = 1;
    for(;;) {
        BaseType_t QopsStatus = errQUEUE_EMPTY;
        QopsStatus = xQueueReceive( xQueue, (void *)&qItemActual, xBlockTime);
        if (QopsStatus == pdPASS) {
            TEST_ASSERT_EQUAL_UINT16(qItemActual, qItemExpected);
            qItemExpected++;
        }
    }
} // end of vQTestKs1Consumer


void vStartQueueTestCase1( UBaseType_t uxPriority )
{
    // note that in most cases, portTICK_PERIOD_MS is usually 1, means the period of
    // time between 2 tick events is 1 millisecond.
    const portSHORT  xDontBlock      = 0;
    const portSHORT  xShortBlockTime = (TickType_t) 1000 / portTICK_PERIOD_MS;
    // collect name for each task
    const portCHAR  Qks1Producer0[0x10] = PROCESS2STR(Qks1Producer0);
    const portCHAR  Qks1Producer1[0x10] = PROCESS2STR(Qks1Producer1);
    const portCHAR  Qks1Producer2[0x10] = PROCESS2STR(Qks1Producer2);
    const portCHAR  Qks1Consumer0[0x10] = PROCESS2STR(Qks1Consumer0);
    const portCHAR  Qks1Consumer1[0x10] = PROCESS2STR(Qks1Consumer1);
    const portCHAR  Qks1Consumer2[0x10] = PROCESS2STR(Qks1Consumer2);
    const portCHAR *pcTaskName[NUM_OF_TASKS] = {
        Qks1Producer0, Qks1Consumer0, Qks1Producer1, Qks1Consumer1,
        Qks1Producer2, Qks1Consumer2
    };
    void (*pvTaskFuncs[NUM_OF_TASKS])(void *) = {
        vQTestKs1Producer, vQTestKs1Consumer, vQTestKs1Producer,
        vQTestKs1Consumer, vQTestKs1Producer, vQTestKs1Consumer
    };
    const portSHORT sQlengthToTask[NUM_OF_TASKS] = {1, 1, 1, 1, 5, 5};
    // collect the blocking time for each task when it performs queue operations.
    portSHORT  sBlkTimeToTask[NUM_OF_TASKS] = {
        xDontBlock, xShortBlockTime, xShortBlockTime,
        xDontBlock, xShortBlockTime, xShortBlockTime
    }; // collect the priority of each task.
    UBaseType_t uxTaskPriority[NUM_OF_TASKS] = {
        (uxPriority     | portPRIVILEGE_BIT),
        ((uxPriority+1) | portPRIVILEGE_BIT),
        ((uxPriority+1) | portPRIVILEGE_BIT),
        (uxPriority     | portPRIVILEGE_BIT),
        (uxPriority     | portPRIVILEGE_BIT),
        (uxPriority     | portPRIVILEGE_BIT)
    }; // collect the parameters used in each
    qCaseParamStruct  *pxQparamsToTask[NUM_OF_TASKS];
    // stack memory for all tasks
    StackType_t       *stackMemSpace[NUM_OF_TASKS] = {0};
    BaseType_t          xState; 
    unsigned portSHORT  idx, jdx;

    for (idx=0; idx<NUM_OF_TASKS; idx++) {
        stackMemSpace[idx] = (StackType_t *) pvPortMalloc( sizeof(StackType_t) * intgSTACK_SIZE );
    }
    for (idx=0; idx<NUM_OF_TASKS; idx++)
    {
        pxQparamsToTask[idx] = (qCaseParamStruct *) pvPortMalloc( sizeof(qCaseParamStruct) );
        configASSERT( pxQparamsToTask[idx] );
        pxQparamsToTask[idx]->xBlockTime = (TickType_t) sBlkTimeToTask;
        // create queues and tasks
        QueueHandle_t q = NULL;
        if(idx % 2 == 0) {
            q = xQueueCreate( sQlengthToTask[idx], (unsigned portBASE_TYPE) sizeof(unsigned portSHORT) );
            configASSERT(q);
        }
        else {
            q = pxQparamsToTask[idx - 1]->xQueue;
        }
        pxQparamsToTask[idx]->xQueue = q;
        TaskParameters_t tskparams = {
            pvTaskFuncs[idx], pcTaskName[idx], intgSTACK_SIZE, (void *)pxQparamsToTask[idx],
            uxTaskPriority[idx], stackMemSpace[idx], 
        }; 
        // default value to unused MPU regions 
        for(jdx=0; jdx<portNUM_CONFIGURABLE_REGIONS; jdx++)
        {
            tskparams.xRegions[jdx].pvBaseAddress   = NULL;
            tskparams.xRegions[jdx].ulLengthInBytes = 0;
            tskparams.xRegions[jdx].ulParameters    = 0;
        }
        xState = xTaskCreateRestricted( (const TaskParameters_t * const)&tskparams, NULL );
        configASSERT( xState == pdPASS );
    } // end of loop
} // end of vStartQueueTestCase1
