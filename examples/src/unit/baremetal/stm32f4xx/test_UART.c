#include "assert.h"
#include "unity_fixture.h"
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim3;

TEST_GROUP(UARTbareMetal);

TEST_SETUP(UARTbareMetal) {
    __HAL_DBGMCU_FREEZE_TIM3();
    __asm volatile(
        "cpsie f  \n" 
        "cpsie i  \n" 
        "isb      \n"
        :::"memory"
    );
    HAL_UART_Init(&huart1);
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(USART1_IRQn);
    HAL_StatusTypeDef result = HAL_TIM_Base_Start_IT(&htim3);
    assert(HAL_OK == result);
}

TEST_TEAR_DOWN(UARTbareMetal) {
    HAL_UART_DeInit(&huart1);
    HAL_StatusTypeDef result = HAL_TIM_Base_Stop_IT(&htim3);
    assert(HAL_OK == result);
    NVIC_DisableIRQ(TIM3_IRQn);
    NVIC_DisableIRQ(USART1_IRQn);
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    __asm volatile(
        "cpsid f  \n" 
        "cpsid i  \n" 
        "isb      \n"
        :::"memory"
    );
    __HAL_DBGMCU_UNFREEZE_TIM3();
}

TEST( UARTbareMetal , loopback_ok ) {
#define RX_BUFFER_SIZE  64
    uint8_t rx_buf[RX_BUFFER_SIZE] = {0};
    uint8_t tx_buf[] = "Llama Pump Chain";
    size_t tx_sz = sizeof(tx_buf) - 1;
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart1, rx_buf, RX_BUFFER_SIZE);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_UART_Transmit(&huart1, tx_buf, tx_sz, 59);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buf, rx_buf, tx_sz);
}
