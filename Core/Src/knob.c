#include "knob.h"

#define KNOB_ADC_CHANNEL          4U
#define KNOB_ADC_SAMPLE_COUNT     8U
#define KNOB_ADC_SAMPLE_TIME_CODE 5U
#define KNOB_ADC_FULL_SCALE       4095.0f
#define KNOB_FILTER_ALPHA         0.20f
#define KNOB_ADC_TIMEOUT          100000U
#define KNOB_ADC_VALID_MIN        20U
#define KNOB_ADC_VALID_MAX        4075U
#define KNOB_ADC_VALID_SPREAD     40U
#define KNOB_VALID_CONFIRM_COUNT  5U

static uint16_t knob_raw = 2048U;
static float knob_position = 0.5f;
static uint8_t knob_ready = 0U;
static uint8_t knob_valid = 0U;
static uint8_t knob_valid_count = 0U;

static uint8_t Knob_WaitFlag(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = KNOB_ADC_TIMEOUT;

    while ((*reg & mask) == 0U) {
        if (timeout-- == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t Knob_WaitClear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = KNOB_ADC_TIMEOUT;

    while ((*reg & mask) != 0U) {
        if (timeout-- == 0U) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t Knob_ReadRaw(uint16_t *raw)
{
    if (raw == NULL) {
        return 0U;
    }

    ADC1->CR2 |= ADC_CR2_ADON;
    if (!Knob_WaitFlag(&ADC1->SR, ADC_SR_EOC)) {
        return 0U;
    }

    *raw = (uint16_t)(ADC1->DR & 0x0FFFU);
    return 1U;
}

uint8_t Knob_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    knob_raw = 2048U;
    knob_position = 0.5f;
    knob_ready = 0U;
    knob_valid = 0U;
    knob_valid_count = 0U;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Keep ADCCLK within the STM32F103 limit: 72 MHz / 6 = 12 MHz. */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_ADCPRE, RCC_CFGR_ADCPRE_DIV6);
    __HAL_RCC_ADC1_CLK_ENABLE();

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;
    ADC1->SQR1 = 0U;
    ADC1->SQR2 = 0U;
    ADC1->SQR3 = KNOB_ADC_CHANNEL;
    ADC1->SMPR2 =
        (ADC1->SMPR2 & ~ADC_SMPR2_SMP4_Msk) |
        (KNOB_ADC_SAMPLE_TIME_CODE << ADC_SMPR2_SMP4_Pos);

    ADC1->CR2 |= ADC_CR2_ADON;
    for (volatile uint32_t i = 0U; i < 1000U; i++) {
    }

    ADC1->CR2 |= ADC_CR2_RSTCAL;
    if (!Knob_WaitClear(&ADC1->CR2, ADC_CR2_RSTCAL)) {
        return 0U;
    }

    ADC1->CR2 |= ADC_CR2_CAL;
    if (!Knob_WaitClear(&ADC1->CR2, ADC_CR2_CAL)) {
        return 0U;
    }

    return 1U;
}

void Knob_Update(void)
{
    uint32_t sum = 0U;
    uint8_t valid_samples = 0U;
    uint16_t sample_min = 4095U;
    uint16_t sample_max = 0U;

    for (uint8_t i = 0U; i < KNOB_ADC_SAMPLE_COUNT; i++) {
        uint16_t sample = 0U;
        if (Knob_ReadRaw(&sample)) {
            sum += sample;
            valid_samples++;
            if (sample < sample_min) {
                sample_min = sample;
            }
            if (sample > sample_max) {
                sample_max = sample;
            }
        }
    }

    if (valid_samples == 0U) {
        return;
    }

    uint16_t raw = (uint16_t)(sum / valid_samples);
    uint16_t sample_spread = (uint16_t)(sample_max - sample_min);
    uint8_t raw_valid = (raw >= KNOB_ADC_VALID_MIN &&
                         raw <= KNOB_ADC_VALID_MAX &&
                         sample_spread <= KNOB_ADC_VALID_SPREAD);

    if (raw_valid) {
        if (knob_valid_count < KNOB_VALID_CONFIRM_COUNT) {
            knob_valid_count++;
        }
    } else {
        knob_valid_count = 0U;
    }
    knob_valid =
        (knob_valid_count >= KNOB_VALID_CONFIRM_COUNT) ? 1U : 0U;

    float position = (float)raw / KNOB_ADC_FULL_SCALE;

    if (position > 1.0f) {
        position = 1.0f;
    } else if (position < 0.0f) {
        position = 0.0f;
    }

    if (knob_ready) {
        knob_position += KNOB_FILTER_ALPHA * (position - knob_position);
    } else {
        knob_position = position;
        knob_ready = 1U;
    }

    knob_raw = raw;
}

float Knob_GetPosition(void)
{
    return knob_position;
}

uint16_t Knob_GetRaw(void)
{
    return knob_raw;
}

uint8_t Knob_IsReady(void)
{
    return (uint8_t)(knob_ready && knob_valid);
}

uint8_t Knob_IsValid(void)
{
    return knob_valid;
}
