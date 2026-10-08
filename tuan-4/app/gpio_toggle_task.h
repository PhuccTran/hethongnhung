#ifndef GPIO_TOGGLE_TASK_H
#define GPIO_TOGGLE_TASK_H

#include <stdint.h>

typedef struct
{
    uint8_t pin_number;
    uint32_t frequency_hz;
} gpio_toggle_task_config_t;

void gpio_toggle_task(void *argument);

#endif
