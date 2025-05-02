#include "assert.h"
#include "unity_fixture.h"
#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef  hspi1, hspi2;
extern TIM_HandleTypeDef  htim3;
extern DMA_HandleTypeDef hdma_spi1_tx, hdma_spi1_rx, hdma_spi2_tx, hdma_spi2_rx;

static uint8_t operation_done, is_fullduplex, spi1_is_sender;

static void ut_spi_start_dma(void) {
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_DMA_Init(&hdma_spi2_tx);
    assert(HAL_OK == result);
    result = HAL_DMA_Init(&hdma_spi2_rx);
    assert(HAL_OK == result);
    result = HAL_DMA_Init(&hdma_spi1_tx);
    assert(HAL_OK == result);
    result = HAL_DMA_Init(&hdma_spi1_rx);
    assert(HAL_OK == result);
    // Link DMA handles to SPI1 handle
    __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
    __HAL_LINKDMA(&hspi1, hdmarx, hdma_spi1_rx);
    __HAL_LINKDMA(&hspi2, hdmarx, hdma_spi2_rx);
    __HAL_LINKDMA(&hspi2, hdmatx, hdma_spi2_tx);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}

static void ut_spi_stop_dma(void) {
    NVIC_DisableIRQ(DMA2_Stream0_IRQn);
    NVIC_DisableIRQ(DMA2_Stream3_IRQn);
    NVIC_DisableIRQ(DMA1_Stream3_IRQn);
    NVIC_DisableIRQ(DMA1_Stream4_IRQn);
    hspi1.hdmatx = NULL;
    hspi1.hdmarx = NULL;
    hspi2.hdmarx = NULL;
    hspi2.hdmatx = NULL;
    HAL_DMA_DeInit(&hdma_spi2_tx);
    HAL_DMA_DeInit(&hdma_spi2_rx);
    HAL_DMA_DeInit(&hdma_spi1_tx);
    HAL_DMA_DeInit(&hdma_spi1_rx);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        operation_done |= 1;
    } else if (hspi->Instance == SPI2) {
        operation_done |= (1 << 1);
    }
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        if(hspi->pTxBuffPtr[0] == 0x0) {
            operation_done |= 1;
        } // end of string to transfer
    }
}
// FIXME error happened before Rx complete callback
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1 || hspi->Instance == SPI2) {
        // in this test case, program should never reach here,
        // cuz Rx buffer always larger than Tx
        assert(0);
    }
}
void App_SPI_ContinueCallback(SPI_HandleTypeDef *hspi) {
    SPI_HandleTypeDef *sender = NULL;
    if (is_fullduplex) {
        return;
    } else if (spi1_is_sender && hspi->Instance == SPI2) {
        sender = &hspi1;
    } else {
        return;
    }
    uint8_t *tx_buf_remaining = sender->pTxBuffPtr;
    assert(tx_buf_remaining);
    // this occurs when `HAL_SPI_Receive_IT` internally invokes
    // `HAL_SPI_TransmitReceive_IT` for SPI1 as master and  receiver
    if(tx_buf_remaining[0] != 0x0) {
        HAL_StatusTypeDef status = HAL_SPI_Transmit_IT(sender, tx_buf_remaining, 0x1);
        assert(HAL_OK == status);
    } else { // tx_buf_remaining[0] == 0x0)
        operation_done |= (1 << 1);
    } // end of string to transfer
}

TEST_GROUP(SPIbareMetal);

static uint32_t tim3_itr_prio_orig[2];

TEST_SETUP(SPIbareMetal) {
    operation_done = 0;
    __HAL_DBGMCU_FREEZE_TIM3();
    __asm volatile(
        "cpsie f  \n" 
        "cpsie i  \n" 
        "isb      \n"
        :::"memory"
    );
    // the callback `SPI_DMATransmitReceiveCplt` invoked by DMA interrupt
    // will require HAL time tick to continuously count up without being
    // blocked due to NVIC priority issue, in DMA related test cases, it is
    // necessary to modify TIM3 priority temporarily.
    HAL_NVIC_GetPriority(
        TIM3_IRQn, NVIC_PRIORITYGROUP_4, &tim3_itr_prio_orig[0],
        &tim3_itr_prio_orig[1]
    );
    uint32_t dma_itr_prio = 0;
    HAL_NVIC_GetPriority(DMA1_Stream3_IRQn, NVIC_PRIORITYGROUP_4, &dma_itr_prio, NULL);
    HAL_NVIC_SetPriority(TIM3_IRQn, dma_itr_prio - 1, 0);
    HAL_StatusTypeDef status = HAL_TIM_Base_Start_IT(&htim3);
    assert(HAL_OK == status);
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(SPI1_IRQn);
    NVIC_EnableIRQ(SPI2_IRQn);
    HAL_SPI_Init(&hspi1);
    HAL_SPI_Init(&hspi2);
    // hspi1.Instance->CR1 &= ~SPI_CR1_SSI; // CPU will hang , FIXME
    // hspi2.Instance->CR1 &= ~SPI_CR1_SSI;
}

TEST_TEAR_DOWN(SPIbareMetal) {
    // hspi1.Instance->CR1 |= SPI_CR1_SSI;
    // hspi2.Instance->CR1 |= SPI_CR1_SSI;
    ut_spi_stop_dma();
    HAL_SPI_Abort_IT(&hspi1);
    HAL_SPI_Abort_IT(&hspi2);
    HAL_SPI_DeInit(&hspi1);
    HAL_SPI_DeInit(&hspi2);
    HAL_StatusTypeDef result = HAL_TIM_Base_Stop_IT(&htim3);
    assert(HAL_OK == result);
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    NVIC_DisableIRQ(SPI1_IRQn);
    NVIC_DisableIRQ(SPI2_IRQn);
    NVIC_DisableIRQ(TIM3_IRQn);
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    HAL_NVIC_SetPriority(TIM3_IRQn, tim3_itr_prio_orig[0], tim3_itr_prio_orig[1]);
    __asm volatile(
        "cpsid f  \n" 
        "cpsid i  \n" 
        "isb      \n"
        :::"memory"
    );
    __HAL_DBGMCU_UNFREEZE_TIM3();
    is_fullduplex = 0;
    spi1_is_sender = 0;
}

#define  MSG0  "Salmon burger dipping sauce"
#define  MSG1  "Potato cute doge daemon owl"
#define  RX_BUF_SZ  32 // sizeof(MSG0)

TEST( SPIbareMetal , loopback_fullduplex_blocking ) {
    uint8_t actual_msgs[2][RX_BUF_SZ] = {0};
    HAL_StatusTypeDef status = HAL_OK;
    is_fullduplex = 1;
    status = HAL_SPI_TransmitReceive_IT(&hspi2, (uint8_t *)MSG0, actual_msgs[0], sizeof(MSG0));
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)MSG1, actual_msgs[1], sizeof(MSG1), 65);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG1, actual_msgs[0], sizeof(MSG1) - 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG0, actual_msgs[1], sizeof(MSG0) - 1);
}

TEST( SPIbareMetal , loopback_master2slave ) {
    uint8_t actual_msg[RX_BUF_SZ] = {0};
    HAL_StatusTypeDef status = HAL_OK;
    spi1_is_sender = 1;
    status = HAL_SPI_Receive_IT(&hspi2, actual_msg, RX_BUF_SZ);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_SPI_Transmit_IT(&hspi1, (uint8_t *)MSG1, 0x1);
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    while(operation_done!= 0x3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG1, actual_msg, sizeof(MSG1) - 1);
}

// TODO, more DMA test with FIFO mode and peripheral burst enabled
TEST( SPIbareMetal , loopback_fullduplex_dma ) {
    uint8_t actual_msgs[2][RX_BUF_SZ] = {0};
    HAL_StatusTypeDef status = HAL_OK;
    ut_spi_start_dma();
    is_fullduplex = 1;
    status = HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t *)MSG0, actual_msgs[0], sizeof(MSG0));
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);
    status = HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t *)MSG1, actual_msgs[1], sizeof(MSG1));
    TEST_ASSERT_EQUAL_UINT32((uint32_t) status, (uint32_t)HAL_OK);

    while(operation_done!= 0x3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG1, actual_msgs[0], sizeof(MSG1) - 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(MSG0, actual_msgs[1], sizeof(MSG0) - 1);
}

#undef  RX_BUF_SZ
#undef  MSG0
#undef  MSG1
