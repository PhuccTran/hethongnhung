#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gpio_toggle_task.h"

#define LED_TASK_STACK_SIZE 512U

static const gpio_toggle_task_config_t led_config = {
    .pin_number = 13U,
    .frequency_hz = 1U
};
static const gpio_toggle_task_config_t led_config2 = {
    .pin_number = 14U,
    .frequency_hz = 10U
};

int main(void)
{
    xTaskCreate(
        gpio_toggle_task,              // Hàm task sẽ được FreeRTOS gọi
        "GPIO_pin_1",                  // Tên task, dùng khi debug
        LED_TASK_STACK_SIZE,           // Kích thước stack: 512 words
        (void *) &led_config,          // Tham số truyền vào task
        tskIDLE_PRIORITY + 1U,         // Độ ưu tiên của task
        NULL                           // Không cần lưu TaskHandle
    );
    xTaskCreate(
        gpio_toggle_task,              // Hàm task sẽ được FreeRTOS gọi
        "GPIO_pin_2",                  // Tên task, dùng khi debug
        LED_TASK_STACK_SIZE,           // Kích thước stack: 512 words
        (void *) &led_config2,         // Tham số truyền vào task
        tskIDLE_PRIORITY + 1U,         // Độ ưu tiên của task
        NULL                           // Không cần lưu TaskHandle
    );
    vTaskStartScheduler();

    for (;;)
    {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void) task;
    (void) task_name;
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

