#include "unity_fixture.h"
#include "stm32f4xx_hal.h"

extern DAC_HandleTypeDef hdac1;
extern ADC_HandleTypeDef hadc1;

TEST_GROUP(AnalogDigitalConvertor);

TEST_SETUP(AnalogDigitalConvertor) {
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
}

TEST_TEAR_DOWN(AnalogDigitalConvertor) {
    HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
}

TEST(AnalogDigitalConvertor, loopback_ok) {
    const uint32_t patterns[] = {0, 512, 1023, 511, 2048, 3072, 789, 1024, 4095};
    const uint32_t margin = 0x10;  // Acceptable ADC error (adjust as needed)
    size_t num_patts_in_margins = 0;
    size_t num_patts = sizeof(patterns)/sizeof(patterns[0]);

    for (uint32_t i = 0; i < num_patts; ++i) {
        uint32_t expected_value = patterns[i]; 
        HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, expected_value);
        volatile uint32_t j, num_samples = 8, sampled_total = 0;
        for (j = 0; j < 1000; ++j);

        for (j = 0; j < num_samples; ++j) {
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
            uint32_t sampled_once  = HAL_ADC_GetValue(&hadc1);
            sampled_total += sampled_once;
        }
        uint32_t sampled_avg = sampled_total / num_samples;
        int diff = (int) sampled_avg - (int) expected_value;
        if((uint32_t)abs(diff) <= margin) {
            num_patts_in_margins++;
        }
    }
    TEST_ASSERT_LESS_THAN( num_patts_in_margins, (num_patts >> 1) );
} // end of test body
