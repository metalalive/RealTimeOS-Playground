#include <assert.h>
#include <string.h>
#include "unity_fixture.h"
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef  hi2c1, hi2c3;
extern DMA_HandleTypeDef hdma_i2c1_tx, hdma_i2c1_rx, hdma_i2c3_tx, hdma_i2c3_rx;

static uint8_t operation_done;

static void ut_i2c_start_dma(void) {
    HAL_DMA_Init(&hdma_i2c1_tx);
    HAL_DMA_Init(&hdma_i2c1_rx);
    HAL_DMA_Init(&hdma_i2c3_tx);
    HAL_DMA_Init(&hdma_i2c3_rx);
    // Link DMA handles to I2C1 handle
    __HAL_LINKDMA(&hi2c1, hdmatx, hdma_i2c1_tx);
    __HAL_LINKDMA(&hi2c1, hdmarx, hdma_i2c1_rx);
    __HAL_LINKDMA(&hi2c3, hdmatx, hdma_i2c3_tx);
    __HAL_LINKDMA(&hi2c3, hdmarx, hdma_i2c3_rx);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}
static void ut_i2c_stop_dma(void) {
    HAL_NVIC_DisableIRQ(DMA1_Stream6_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Stream4_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn);
    hi2c1.hdmatx = NULL;
    hi2c1.hdmarx = NULL;
    hi2c3.hdmatx = NULL;
    hi2c3.hdmarx = NULL;
    HAL_DMA_DeInit(&hdma_i2c1_tx);
    HAL_DMA_DeInit(&hdma_i2c1_rx);
    HAL_DMA_DeInit(&hdma_i2c3_tx);
    HAL_DMA_DeInit(&hdma_i2c3_rx);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        operation_done |= 0x1;
    } else if (hi2c->Instance == I2C3) {
        operation_done |= 0x4;
    }
}
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C3) {
        operation_done |= 0x2;
    } else if (hi2c->Instance == I2C1) {
        operation_done |= 0x8;
    }
}
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        operation_done |= 0x10;
    }
}
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C3) {
        operation_done |= 0x20;
    }
} // invoked from I2C3_ER_IRQHandler

TEST_GROUP(I2CbareMetal);

TEST_SETUP(I2CbareMetal) {
    __asm volatile(
        "cpsie f  \n" 
        "cpsie i  \n" 
        "isb      \n"
        :::"memory"
    );
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_I2C_Init(&hi2c1);
    assert(status == HAL_OK);
    status = HAL_I2C_Init(&hi2c3);
    assert(status == HAL_OK);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);
    operation_done = 0x0;
}

TEST_TEAR_DOWN(I2CbareMetal) {
    ut_i2c_stop_dma();
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    HAL_NVIC_DisableIRQ(I2C3_ER_IRQn);
    HAL_I2C_DeInit(&hi2c1);
    HAL_I2C_DeInit(&hi2c3);
    __asm volatile(
        "cpsid f  \n" 
        "cpsid i  \n" 
        "isb      \n"
        :::"memory"
    );
}

#define  MSG0  "Eye SquareEr Ssea, friendly face is everywhere"
#define  MSG1  "Ai SquercyeEe, Humble Folks without temptation"
#define  RX_BUF_SZ  sizeof(MSG0)

TEST( I2CbareMetal , loopback_nonblocking_itr ) {
    uint8_t actual_msg[RX_BUF_SZ] = {0};
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_I2C_Slave_Receive_IT(&hi2c3, actual_msg, RX_BUF_SZ - 1);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_I2C_Master_Transmit_IT(
        &hi2c1, hi2c3.Init.OwnAddress1, (uint8_t *)MSG0, RX_BUF_SZ - 1
    );
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    while(operation_done!= 0x3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG0, actual_msg, sizeof(MSG0) - 1);
    // subcase 2, exchange master slave roles
    operation_done = 0x0;
    status = HAL_I2C_Slave_Receive_IT(&hi2c1, actual_msg, RX_BUF_SZ - 1);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_I2C_Master_Transmit_IT(
        &hi2c3, hi2c1.Init.OwnAddress1, (uint8_t *)MSG1, RX_BUF_SZ - 1
    );
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    while(operation_done!= 0xc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG1, actual_msg, sizeof(MSG1) - 1);
} // end of test body

// TODO, more DMA test with FIFO mode and peripheral burst enabled
TEST( I2CbareMetal , loopback_nonblocking_dma ) {
    ut_i2c_start_dma();
    uint8_t actual_msg[RX_BUF_SZ] = {0};
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_I2C_Slave_Receive_DMA(&hi2c3, actual_msg, RX_BUF_SZ - 1);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_I2C_Master_Transmit_DMA(
        &hi2c1, hi2c3.Init.OwnAddress1, (uint8_t *)MSG0, RX_BUF_SZ - 1
    );
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    while(operation_done!= 0x3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG0, actual_msg, sizeof(MSG0) - 1);

    operation_done = 0x0;
    memset(actual_msg, 0x0, sizeof(char) * RX_BUF_SZ);
    // subcase 2
    // The slave’s TX DMA must be armed first so its DR is loaded before the master clocks data
    status = HAL_I2C_Slave_Transmit_DMA(&hi2c3, (uint8_t *)MSG1, RX_BUF_SZ - 1);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_I2C_Master_Receive_DMA(
        &hi2c1, hi2c3.Init.OwnAddress1, actual_msg, RX_BUF_SZ - 1
    );
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    while(operation_done!= 0x30); // FIXME , flags should be 0x30
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG1, actual_msg, sizeof(MSG1) - 1);
} // end of test body

#undef  MSG0
#undef  RX_BUF_SZ
