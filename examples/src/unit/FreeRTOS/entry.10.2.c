/* USER CODE BEGIN Header */
/* USER CODE END Header */

#include "unity_fixture.h"

// hardware-specific Initialization
void hw_layer_init(void);

static void RunAllTestGroups( void ) {
    RUN_TEST_GROUP( FreeRTOS_v10_2_port );
}

static void TestEnd(void) {
    while(1);
}

int main(void) {
    hw_layer_init();
    // start our unit test framework without arguments
    UnityMain( 0, NULL, RunAllTestGroups );
    TestEnd();
}
