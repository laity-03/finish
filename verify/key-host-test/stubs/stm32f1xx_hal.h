/*
 * Minimal STM32 HAL subset so that the real key.c / control.c logic can be
 * compiled and executed on the host (Windows PC) for behavior tests.
 */
#ifndef TEST_STM32F1XX_HAL_H
#define TEST_STM32F1XX_HAL_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int unused;
} GPIO_TypeDef;

extern GPIO_TypeDef test_gpio_a;
extern GPIO_TypeDef test_gpio_b;
extern GPIO_TypeDef test_gpio_c;
extern GPIO_TypeDef test_gpio_d;
extern GPIO_TypeDef test_gpio_e;

#define GPIOA (&test_gpio_a)
#define GPIOB (&test_gpio_b)
#define GPIOC (&test_gpio_c)
#define GPIOD (&test_gpio_d)
#define GPIOE (&test_gpio_e)

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET = 1
} GPIO_PinState;

#define GPIO_MODE_INPUT 0x00000000U
#define GPIO_PULLUP     0x00000001U

#define GPIO_PIN_2  ((uint16_t)0x0004U)
#define GPIO_PIN_3  ((uint16_t)0x0008U)
#define GPIO_PIN_12 ((uint16_t)0x1000U)
#define GPIO_PIN_13 ((uint16_t)0x2000U)

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

#define __HAL_RCC_GPIOA_CLK_ENABLE() do { } while (0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() do { } while (0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() do { } while (0)
#define __HAL_RCC_GPIOD_CLK_ENABLE() do { } while (0)
#define __HAL_RCC_GPIOE_CLK_ENABLE() do { } while (0)

#endif /* TEST_STM32F1XX_HAL_H */
