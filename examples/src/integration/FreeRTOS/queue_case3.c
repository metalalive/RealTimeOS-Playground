#include "queue_case3.h"

#define  NUM_OF_TASKS    2
#define  SHARED_Q_LENGTH         7
// in our case, we create tasks' stack using privileged function  pvPortMalloc() 
// implemented in heap_4.c, each allocated block memory preserves first 8 bytes 
// for  internal data structure, representing size of the allocated block.
#define  intgSTACK_SIZE      (( unsigned portSHORT ) 0x3e)
#define  uxTASK_DELAY           40

// the data structure below is used only for passing parameters to the tasks in this tests
typedef struct {
    // the queue used by the tasks
    QueueHandle_t        xQueue; 
    // the blocking time can be applied to Queue send/receive operations, or task delay functions
    TickType_t           xBlockTime;
} qCaseParamStruct;

// to add a bit of complexity, all the items, which are sent to the shared queue, are
// from following arrays
static volatile const portSHORT  possibleQItemValue[SHARED_Q_LENGTH] = {77 , 28, -15, 193, -14, 97, -129};
// declare the structure outside any function , since the shared queue will be accessed by 
// interrupt service routine
static volatile qCaseParamStruct  *pxQparamsToTask[NUM_OF_TASKS] ;

static volatile unsigned portSHORT tsk1syncSenderIdx   = 0;
static volatile unsigned portSHORT tsk1syncRecverIdx   = 0;
static volatile unsigned portSHORT tsk2syncSenderIdx   = 2;
static volatile unsigned portSHORT tsk2syncRecverIdx   = 2;


void vQueueTestCase3ISR1(void) {
    // remind this ISR and vQks3tsk1() share the same queue.
    qCaseParamStruct *qParams = pxQparamsToTask[0];
    QueueHandle_t     xQueue  = qParams->xQueue;
    portSHORT qItem = 0;
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE; 
    // read out one item from the shared queue
    BaseType_t QopsStatus = xQueueReceiveFromISR( xQueue, (void *)&qItem, &pxHigherPriorityTaskWoken );
    if(QopsStatus == pdPASS) {
        configASSERT(qItem == possibleQItemValue[tsk1syncRecverIdx]);
        tsk1syncRecverIdx = (tsk1syncRecverIdx + 1) % SHARED_Q_LENGTH;
    }
    // perform context switch if any higher priority task is woken due to xQueueReceiveFromISR() above
    portEND_SWITCHING_ISR( pxHigherPriorityTaskWoken );
}


void vQueueTestCase3ISR2(void) {
    // remind this ISR and vQks3tsk2() share the same queue.
    qCaseParamStruct *qParams = pxQparamsToTask[1];
    QueueHandle_t     xQueue  = qParams->xQueue;
    portSHORT  qItem = possibleQItemValue[tsk2syncSenderIdx]; 
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE; 
    // send one item to the shared queue
    BaseType_t QopsStatus = xQueueSendToBackFromISR( xQueue, (void *)&qItem, &pxHigherPriorityTaskWoken );
    if(QopsStatus == pdPASS) {
        tsk2syncSenderIdx = (tsk2syncSenderIdx + 1) % SHARED_Q_LENGTH;
    }
    // perform context switch if any higher priority task is woken due to xQueueReceiveFromISR() above
    portEND_SWITCHING_ISR( pxHigherPriorityTaskWoken );
}


static void vQks3tsk1(void *pvParams) {
    qCaseParamStruct  *qParams = (qCaseParamStruct *) pvParams;
    QueueHandle_t       xQueue = qParams->xQueue;
    TickType_t      xBlockTime = qParams->xBlockTime;
    for(;;) {
        // temporarily disable interrupt to prevent race condition between
        // this task and ISR
        portDISABLE_INTERRUPTS();
        portSHORT  qItem = possibleQItemValue[ tsk1syncSenderIdx ];
        BaseType_t QopsStatus = xQueueSendToBack(xQueue, (void *)&qItem, xBlockTime);
        if(QopsStatus == pdPASS) {
            tsk1syncSenderIdx = (tsk1syncSenderIdx + 1) % SHARED_Q_LENGTH;
        }
        portENABLE_INTERRUPTS();
        vTaskDelay( xBlockTime );
    }
}

static void vQks3tsk2(void *pvParams) {
    qCaseParamStruct  *qParams = (qCaseParamStruct *) pvParams;
    QueueHandle_t       xQueue = qParams->xQueue;
    TickType_t      xBlockTime = qParams->xBlockTime;
    for(;;) {
        portSHORT qItem = 0;
        portDISABLE_INTERRUPTS();
        BaseType_t QopsStatus = xQueueReceive(xQueue, (void *)&qItem, xBlockTime);
        if(QopsStatus == pdPASS) {
            configASSERT(qItem == possibleQItemValue[tsk2syncRecverIdx]);
            tsk2syncRecverIdx = (tsk2syncRecverIdx + 1) % SHARED_Q_LENGTH;
        }
        portENABLE_INTERRUPTS();
        vTaskDelay( xBlockTime );
    }
}

void vStartQueueTestCase3( UBaseType_t uxPriority )
{
    StackType_t        *stackMemSpace[NUM_OF_TASKS] ;
    BaseType_t          xState; 
    unsigned portSHORT  idx;

    pxQparamsToTask[0] = (qCaseParamStruct *) pvPortMalloc( sizeof(qCaseParamStruct) );
    pxQparamsToTask[1] = (qCaseParamStruct *) pvPortMalloc( sizeof(qCaseParamStruct) );
    configASSERT( pxQparamsToTask[0] );
    configASSERT( pxQparamsToTask[1] );

    pxQparamsToTask[0]->xBlockTime  = uxTASK_DELAY;
    pxQparamsToTask[1]->xBlockTime  = uxTASK_DELAY << 2 ;
    pxQparamsToTask[0]->xQueue      = xQueueCreate( SHARED_Q_LENGTH, (unsigned portBASE_TYPE) sizeof(portSHORT) );
    pxQparamsToTask[1]->xQueue      = xQueueCreate( SHARED_Q_LENGTH, (unsigned portBASE_TYPE) sizeof(portSHORT) );
    configASSERT( pxQparamsToTask[0]->xQueue );
    configASSERT( pxQparamsToTask[1]->xQueue );
    for (idx=0; idx<NUM_OF_TASKS; idx++) {
        stackMemSpace[idx] = (StackType_t *) pvPortMalloc( sizeof(StackType_t) * intgSTACK_SIZE );
    }
    TaskParameters_t tsk1params = {
        vQks3tsk1, "Qks3tsk1", intgSTACK_SIZE, (void *)pxQparamsToTask[0],
        (uxPriority | portPRIVILEGE_BIT), stackMemSpace[0],
    };
    TaskParameters_t tsk2params = {
        vQks3tsk2, "Qks3tsk2", intgSTACK_SIZE, (void *)pxQparamsToTask[1],
        (uxPriority | portPRIVILEGE_BIT), stackMemSpace[1],
    };
    // default value to unused MPU regions 
    for(idx=0; idx<portNUM_CONFIGURABLE_REGIONS; idx++)
    {
        tsk1params.xRegions[idx].pvBaseAddress   = NULL;
        tsk1params.xRegions[idx].ulLengthInBytes = 0;
        tsk1params.xRegions[idx].ulParameters    = 0;
        tsk2params.xRegions[idx].pvBaseAddress   = NULL;
        tsk2params.xRegions[idx].ulLengthInBytes = 0;
        tsk2params.xRegions[idx].ulParameters    = 0;
    }
    xState = xTaskCreateRestricted( (const TaskParameters_t * const)&tsk1params, NULL );
    configASSERT( xState == pdPASS );
    xState = xTaskCreateRestricted( (const TaskParameters_t * const)&tsk2params, NULL );
    configASSERT( xState == pdPASS );

    // xTaskCreate( vQks3tsk1, "Qks3tsk1", intgSTACK_SIZE, (void *)pxQparamsToTask[0], uxPriority, NULL);
    // xTaskCreate( vQks3tsk2, "Qks3tsk2", intgSTACK_SIZE, (void *)pxQparamsToTask[1], uxPriority, NULL);
} // end of vStartQueueTestCase3()
