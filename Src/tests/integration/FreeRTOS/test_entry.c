#include "generic.h"
#include "test_runner.h"

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
