/* USER CODE BEGIN Header */
/* USER CODE END Header */

#include "test_entry.h"

// hardware-specific Initialization
void hw_layer_init(void);

static void TestEnd(void) {
    while(1);
}

int main(void) {
    hw_layer_init();
    // start the integration tests
    vCreateAllTestTasks();
    vTaskStartScheduler();
    TestEnd();
}
