/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "assert.h"
#include "hal_init.h"
#include "stm32f4xx_it.h"

// implemented at RTOS layer
void vRTOSTimer3ISR(void);
void vRTOSTimer4ISR(void);
int  vRTOSMemManageHandler(void);
void vRTOSSysTickHandler(void);

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim3, htim4;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef  hspi1, hspi2;
extern I2C_HandleTypeDef  hi2c1, hi2c3;
extern DMA_HandleTypeDef hdma_spi1_tx, hdma_spi1_rx, hdma_spi2_tx, hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_i2c1_tx, hdma_i2c1_rx, hdma_i2c3_tx, hdma_i2c3_rx;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */ 
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void) {
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

void MemManage_Handler(void) {
    int alreadyHandled = vRTOSMemManageHandler();
    if(!alreadyHandled) {
        while (1);
    }
}

void BusFault_Handler(void) {
  /* USER CODE BEGIN BusFault_IRQn 0 */
  /* USER CODE END BusFault_IRQn 0 */
  while (1) {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

void UsageFault_Handler(void) {
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */
  /* USER CODE END DebugMonitor_IRQn 1 */
}

void SysTick_Handler(void) {
    vRTOSSysTickHandler();
}


/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line 4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */
  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  /* USER CODE BEGIN EXTI4_IRQn 1 */
  /* USER CODE END EXTI4_IRQn 1 */
}

// overwrite weak function declarations in other files
void TIM3_IRQHandler(void) {
    vRTOSTimer3ISR();
    HAL_TIM_IRQHandler(&htim3);
}

void TIM4_IRQHandler(void) {
    vRTOSTimer4ISR();
    HAL_TIM_IRQHandler(&htim4);
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

__weak void App_SPI_ContinueCallback(SPI_HandleTypeDef *hspi) {
}

void SPI1_IRQHandler(void) {
    HAL_SPI_IRQHandler(&hspi1);
    App_SPI_ContinueCallback(&hspi1);
}
void SPI2_IRQHandler(void) {
    HAL_SPI_IRQHandler(&hspi2);
    App_SPI_ContinueCallback(&hspi2);
}

void I2C1_EV_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&hi2c1);
}
void I2C3_EV_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&hi2c3);
}
void I2C1_ER_IRQHandler(void) {
    HAL_I2C_ER_IRQHandler(&hi2c1);
}
void I2C3_ER_IRQHandler(void) {
    HAL_I2C_ER_IRQHandler(&hi2c3);
}

void DMA2_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi1_rx);
}
void DMA2_Stream3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi1_tx);
}
void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}
void DMA1_Stream1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c3_rx);
}
void DMA1_Stream3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

#if 0
static uint32_t dbg_i2c3_tx_dma[4] = {0};
static uint32_t dbg_dma_feif_cnt = 0;

static void dbg_snapshot_dma1_stream4(void) {
    uint32_t tmp_hisr = DMA1->HISR;
    uint32_t mskerr_hisr = 0x4f;
    uint32_t errs_hisr = tmp_hisr & mskerr_hisr;
    uint32_t tmp_SxCR = DMA1_Stream4->CR;
    if(errs_hisr != 0) {
        dbg_i2c3_tx_dma[0] = tmp_hisr;
        dbg_i2c3_tx_dma[1] = tmp_SxCR;
        dbg_i2c3_tx_dma[2] = DMA1_Stream4->NDTR;
        dbg_i2c3_tx_dma[3] = DMA1_Stream4->FCR;
        if (chn == DMA_CHANNEL_3) {
            if((errs_hisr & DMA_HISR_FEIF4) != RESET) {
                dbg_dma_feif_cnt++;
            }
        }
    }
}
#endif

void DMA1_Stream4_IRQHandler(void) {
    /*
     Single IRQ line, multiple handles: Both hdma_spi2_tx and hdma_i2c3_tx
     share DMA1_Stream4, so the NVIC only raises one IRQ.

     By calling HAL_DMA_IRQHandler() on each handle in turn, the HAL driver’s
     flag‐testing macros (__HAL_DMA_GET_FLAG) detect which peripheral’s
     transfer has actually completed or errored, clear the relevant bits, and
     invoke the corresponding HAL callbacks (HAL_SPI_TxCpltCallback or
     HAL_I2C_MasterTxCpltCallback)
     * */
    uint32_t tmp_SxCR = DMA1_Stream4->CR;
    uint32_t chn = tmp_SxCR & DMA_SxCR_CHSEL;
#if 0
    dbg_snapshot_dma1_stream4();
#endif
    // FIXME, TODO
    // FIFO error interrupt might occurs occasionally due to timing issue at hardware level
    // where the memory bus has not been granted before a peripheral request occurs
    switch (chn) {
        case DMA_CHANNEL_0:
            HAL_DMA_IRQHandler(&hdma_spi2_tx);
            break;
        case DMA_CHANNEL_3:
            HAL_DMA_IRQHandler(&hdma_i2c3_tx);
            break;
        default:
            assert(0);
    }
}
void DMA1_Stream6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
