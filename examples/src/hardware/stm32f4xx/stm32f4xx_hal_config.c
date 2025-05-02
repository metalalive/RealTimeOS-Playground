#include "generic.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOSConfig.h"

TIM_HandleTypeDef htim3, htim4;
DAC_HandleTypeDef hdac1;
ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart1;
SPI_HandleTypeDef  hspi1, hspi2;
I2C_HandleTypeDef  hi2c1, hi2c3;
DMA_HandleTypeDef hdma_spi1_tx, hdma_spi1_rx, hdma_spi2_tx, hdma_spi2_rx;
DMA_HandleTypeDef hdma_i2c1_tx, hdma_i2c1_rx, hdma_i2c3_tx, hdma_i2c3_rx;

static void Error_Handler(void) {
    /* User can add his own implementation to report the HAL error return state */
    while(1);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    // Configure the main internal regulator output voltage
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
    // Initializes the CPU, AHB and APB busses clocks
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    // Initializes the CPU, AHB and APB busses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pins : PB5 as source of other input device */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    // normally the input pin should be set to `GPIO_NOPULL`, let external
    // input device pull the state , since PB4 pin defaults (aka input-floating mode
    // after reset) to nJTRST (active-low JTAG reset pin), and this is only for
    // testing purpose, pull-down is set here to avoid pending interrupt
    // affecting other test cases
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 13, 0);
    
    // strangely there is always pending interrupt raising immediately after
    // GPIO initialization .
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);
    
    // set up EXTI interrupt without enabling it, currently this application
    // does not really needs to integrate with external hardware components.
    HAL_NVIC_DisableIRQ(EXTI4_IRQn);
    // HAL_NVIC_SetPriority(EXTI4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 0x1, 0);
    NVIC_ClearPendingIRQ(EXTI4_IRQn);

    // for DAC
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    // for ADC
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    // USART1 GPIO Configuration    
    // PA9    ------> USART1_TX
    // PB7    ------> USART1_RX 
    GPIO_InitStruct.Pin   = GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP; // to prevent idle-line noise
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* SPI1, Configure PA5=SCK, PA6=MISO, PA7=MOSI, PA15=NSS */
    GPIO_InitStruct.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_15;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* SPI2, Configure PB10=SCK, PB14=MISO, PB15=MOSI, PB4=NSS */
    GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = GPIO_PIN_4;
    GPIO_InitStruct.Alternate = GPIO_AF7_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 pins PB6=SCL, PB9=SDA */
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    /* I2C3 pins PA8=SCL,  PC9=SDA */
    GPIO_InitStruct.Pin       = GPIO_PIN_8;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
} // end of MX_GPIO_Init

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        HAL_IncTick();
    }
}

void MX_TIM3_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    uint32_t uwTimclock = 0, uwPrescalerValue = 0;
    
    __HAL_RCC_TIM3_CLK_ENABLE();
    uwTimclock = HAL_RCC_GetPCLK1Freq();
    /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000) + 13);

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = uwPrescalerValue;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000000 / 6 ;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.InterruptGrpPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 0x2;
    htim3.InterruptSubPriority = 0;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_Base_Start_IT(&htim3);
}

void MX_TIM4_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    __HAL_RCC_TIM4_CLK_ENABLE();
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period =  1000000 / 7;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.InterruptGrpPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 0x1;
    htim4.InterruptSubPriority = 0;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_Base_Start_IT(&htim4);
}

static void DAC_Init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();

    hdac1.Instance = DAC;
    HAL_DAC_Init(&hdac1);

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1);
}
static void ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE; // Enable
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/*
 * Note , USART2 cannot be used without physically modifying STM32F446 board
 * by soldering SB62 to SB63 and desoldering SB13 to SB14 .
 * */
static void UART_INIT(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_NVIC_SetPriority(USART1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
}

static void SPI1_Init(void) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES; // implicitly means full-duplex communication
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    // in this demo application , there is only one SPI master and slave,
    // simply connect the NSS pins on both sides.
    hspi1.Init.NSS               = SPI_NSS_HARD_OUTPUT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    HAL_NVIC_SetPriority(SPI1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 3);
}
static void SPI2_Init(void) {
    __HAL_RCC_SPI2_CLK_ENABLE();
    hspi2.Instance          = SPI2;
    hspi2.Init.Mode         = SPI_MODE_SLAVE;
    hspi2.Init.Direction    = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize     = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity  = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase     = SPI_PHASE_1EDGE;
    hspi2.Init.NSS          = SPI_NSS_HARD_INPUT;
    hspi2.Init.FirstBit     = SPI_FIRSTBIT_MSB;
    HAL_NVIC_SetPriority(SPI2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 3);
}

#define I2C_CLK_100KHZ  100000
static void I2C1_Init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = I2C_CLK_100KHZ;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = (0x2d << 1);
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 4);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 4);
}

static void I2C3_Init(void) {
    __HAL_RCC_I2C3_CLK_ENABLE();
    hi2c3.Instance             = I2C3;
    hi2c3.Init.ClockSpeed      = I2C_CLK_100KHZ;
    hi2c3.Init.OwnAddress1     = (0x30 << 1); // 7-bit address 0x30
    hi2c3.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2     = 0;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 4);
    HAL_NVIC_SetPriority(I2C3_ER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 4);
}
#undef  I2C_CLK_100KHZ

static void DMA1_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();
    // Configure DMA for SPI2_Tx
    hdma_spi2_tx.Instance                 = DMA1_Stream4;
    hdma_spi2_tx.Init.Channel             = DMA_CHANNEL_0;
    hdma_spi2_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_spi2_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_spi2_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_spi2_tx.Init.Mode                = DMA_NORMAL;
    hdma_spi2_tx.Init.Priority            = DMA_PRIORITY_HIGH;
    // Configure DMA for SPI2_Rx
    hdma_spi2_rx.Instance                 = DMA1_Stream3;
    hdma_spi2_rx.Init.Channel             = DMA_CHANNEL_0;
    hdma_spi2_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_spi2_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_spi2_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_spi2_rx.Init.Mode                = DMA_NORMAL;
    hdma_spi2_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_i2c1_tx.Instance     = DMA1_Stream6;
    hdma_i2c1_tx.Init.Channel = DMA_CHANNEL_1;
    hdma_i2c1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_i2c1_rx.Instance     = DMA1_Stream0;
    hdma_i2c1_rx.Init.Channel = DMA_CHANNEL_1;
    hdma_i2c1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_i2c3_tx.Instance     = DMA1_Stream4;
    hdma_i2c3_tx.Init.Channel = DMA_CHANNEL_3;
    hdma_i2c3_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2c3_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c3_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c3_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c3_tx.Init.Mode                = DMA_NORMAL;
    hdma_i2c3_tx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_i2c3_rx.Instance     = DMA1_Stream1;
    hdma_i2c3_rx.Init.Channel = DMA_CHANNEL_1;
    hdma_i2c3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c3_rx.Init.MemInc    = DMA_MINC_ENABLE;
    hdma_i2c3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c3_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c3_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_i2c3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
} // end of DMA1_Init

static void DMA2_Init(void) {
    __HAL_RCC_DMA2_CLK_ENABLE();
    // Configure DMA for SPI1 Tx
    // see DMA reques mapping in RM0390 to determine which stream and channel to choose
    hdma_spi1_tx.Instance = DMA2_Stream3;
    hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    // Configure DMA for SPI1 Rx
    hdma_spi1_rx.Instance = DMA2_Stream0;
    hdma_spi1_rx.Init.Channel = DMA_CHANNEL_3;
    hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_rx.Init.Mode = DMA_NORMAL;
    hdma_spi1_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_spi1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
} // end of DMA2_Init

void hw_layer_init(void) {
    /* MCU Configuration--------------------------------------------------------*/
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    
    DAC_Init();
    ADC1_Init();
    UART_INIT();
    SPI1_Init();
    SPI2_Init();
    I2C1_Init();
    I2C3_Init();
    DMA1_Init();
    DMA2_Init();
}
