#include "gpio_toggle_task.h"

#include "FreeRTOS.h"
#include "debug.h"
#include "stm32f10x.h"
#include "task.h"

#define GPIO_TASK_PORT GPIOC
#define GPIO_TASK_RCC_CLOCK RCC_APB2Periph_GPIOC

static uint16_t pin_number_to_mask(uint8_t pin_number)
{
    return (uint16_t) (1UL << pin_number);
}

void gpio_toggle_task(void *argument)
{
    const gpio_toggle_task_config_t *config = (const gpio_toggle_task_config_t *) argument;
    GPIO_InitTypeDef gpio_init;
    uint16_t pin_mask;
    TickType_t half_period;

    if ((config == NULL) ||
        (config->pin_number < 1U) || (config->pin_number > 15U) ||
        (config->frequency_hz == 0U))
    {
        debug_printf("invalid GPIO task config\r\n");
        vTaskDelete(NULL);
        return;
    }

    pin_mask = pin_number_to_mask(config->pin_number);
    half_period = pdMS_TO_TICKS(1000U / (2U * config->frequency_hz));
    if (half_period < 1U)
    {
        half_period = 1U;
    }

    RCC_APB2PeriphClockCmd(GPIO_TASK_RCC_CLOCK, ENABLE);

    gpio_init.GPIO_Pin = pin_mask;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIO_TASK_PORT, &gpio_init);
    GPIO_SetBits(GPIO_TASK_PORT, pin_mask);

    debug_printf("GPIO task pin=%u freq=%luHz\r\n",
                 (unsigned int) config->pin_number,
                 (unsigned long) config->frequency_hz);

    for (;;)
    {
        GPIO_TASK_PORT->ODR ^= pin_mask;
        vTaskDelay(half_period);
    }
}
