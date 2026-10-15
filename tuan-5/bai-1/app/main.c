#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define LED_COUNT               3U
#define PWM_PERIOD              999U
#define BRIGHTNESS_MAX          100U
#define BRIGHTNESS_STEP         10U
#define BUTTON_SAMPLE_MS        10U
#define BUTTON_DEBOUNCE_MS      30U
#define LED_TASK_STACK_SIZE     256U
#define BUTTON_TASK_STACK_SIZE  256U

#define BUTTON_SELECT_PIN GPIO_Pin_12
#define BUTTON_UP_PIN     GPIO_Pin_13
#define BUTTON_DOWN_PIN   GPIO_Pin_14

typedef struct
{
    uint8_t channel;
    QueueHandle_t brightness_queue;
} led_task_config_t;

typedef struct
{
    uint8_t stable_pressed;
    uint8_t candidate_pressed;
    TickType_t candidate_since;
} button_state_t;

static QueueHandle_t led_queues[LED_COUNT];
static led_task_config_t led_configs[LED_COUNT];

static void application_error(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

static void pwm_init(void)
{
    GPIO_InitTypeDef gpio_init;
    TIM_TimeBaseInitTypeDef timer_base;
    TIM_OCInitTypeDef timer_output;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    timer_base.TIM_Prescaler = 71U;
    timer_base.TIM_CounterMode = TIM_CounterMode_Up;
    timer_base.TIM_Period = PWM_PERIOD;
    timer_base.TIM_ClockDivision = TIM_CKD_DIV1;
    timer_base.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &timer_base);

    timer_output.TIM_OCMode = TIM_OCMode_PWM1;
    timer_output.TIM_OutputState = TIM_OutputState_Enable;
    timer_output.TIM_Pulse = 0U;
    timer_output.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &timer_output);
    TIM_OC2Init(TIM2, &timer_output);
    TIM_OC3Init(TIM2, &timer_output);

    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

static void buttons_init(void)
{
    GPIO_InitTypeDef gpio_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio_init.GPIO_Pin = BUTTON_SELECT_PIN | BUTTON_UP_PIN | BUTTON_DOWN_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio_init);
}

static uint16_t brightness_to_pulse(uint8_t brightness_percent)
{
    return (uint16_t)(((uint32_t)brightness_percent * (PWM_PERIOD + 1U)) / 100U);
}

static void publish_selected_brightness(uint8_t selected_led,
                                        const uint8_t brightness[LED_COUNT])
{
    uint8_t brightness_percent = brightness[selected_led];
    if (xQueueOverwrite(led_queues[selected_led], &brightness_percent) != pdPASS)
    {
        application_error();
    }
}

static void led_task(void *argument)
{
    const led_task_config_t *config = (const led_task_config_t *)argument;
    uint8_t brightness_percent;

    for (;;)
    {
        if (xQueueReceive(config->brightness_queue,
                          &brightness_percent,
                          portMAX_DELAY) == pdPASS)
        {
            uint16_t pulse = brightness_to_pulse(brightness_percent);

            switch (config->channel)
            {
                case 0U:
                    TIM_SetCompare1(TIM2, pulse);
                    break;
                case 1U:
                    TIM_SetCompare2(TIM2, pulse);
                    break;
                case 2U:
                    TIM_SetCompare3(TIM2, pulse);
                    break;
                default:
                    application_error();
                    break;
            }
        }
    }
}

static uint8_t button_is_pressed(uint16_t pin)
{
    return GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET ? 1U : 0U;
}

static void handle_button_press(uint8_t button_index,
                                uint8_t *selected_led,
                                uint8_t brightness[LED_COUNT])
{
    if (button_index == 0U)
    {
        *selected_led = (uint8_t)((*selected_led + 1U) % LED_COUNT);
    }
    else if (button_index == 1U)
    {
        if (brightness[*selected_led] < BRIGHTNESS_MAX)
        {
            brightness[*selected_led] =
                (uint8_t)(brightness[*selected_led] + BRIGHTNESS_STEP);
        }
    }
    else if (brightness[*selected_led] > 0U)
    {
        brightness[*selected_led] =
            (uint8_t)(brightness[*selected_led] - BRIGHTNESS_STEP);
    }

    publish_selected_brightness(*selected_led, brightness);
}

static void buttons_task(void *argument)
{
    static const uint16_t button_pins[] = {
        BUTTON_SELECT_PIN,
        BUTTON_UP_PIN,
        BUTTON_DOWN_PIN
    };
    button_state_t states[3];
    uint8_t brightness[LED_COUNT] = {0U, 0U, 0U};
    uint8_t selected_led = 0U;
    uint8_t button;
    TickType_t now;
    const TickType_t debounce_ticks = pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS);

    (void)argument;

    now = xTaskGetTickCount();
    for (button = 0U; button < 3U; ++button)
    {
        uint8_t pressed = button_is_pressed(button_pins[button]);
        states[button].stable_pressed = pressed;
        states[button].candidate_pressed = pressed;
        states[button].candidate_since = now;
    }

    for (;;)
    {
        now = xTaskGetTickCount();
        for (button = 0U; button < 3U; ++button)
        {
            uint8_t pressed = button_is_pressed(button_pins[button]);

            if (pressed != states[button].candidate_pressed)
            {
                states[button].candidate_pressed = pressed;
                states[button].candidate_since = now;
            }
            else if ((pressed != states[button].stable_pressed) &&
                     ((TickType_t)(now - states[button].candidate_since) >= debounce_ticks))
            {
                states[button].stable_pressed = pressed;
                if (pressed != 0U)
                {
                    handle_button_press(button, &selected_led, brightness);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_SAMPLE_MS));
    }
}

int main(void)
{
    uint8_t led;
    BaseType_t result;

    pwm_init();
    buttons_init();

    for (led = 0U; led < LED_COUNT; ++led)
    {
        led_queues[led] = xQueueCreate(1U, sizeof(uint8_t));
        if (led_queues[led] == NULL)
        {
            application_error();
        }

        led_configs[led].channel = led;
        led_configs[led].brightness_queue = led_queues[led];
        result = xTaskCreate(led_task,
                             led == 0U ? "LED_1" : (led == 1U ? "LED_2" : "LED_3"),
                             LED_TASK_STACK_SIZE,
                             &led_configs[led],
                             tskIDLE_PRIORITY + 1U,
                             NULL);
        if (result != pdPASS)
        {
            application_error();
        }
    }

    result = xTaskCreate(buttons_task,
                         "Buttons",
                         BUTTON_TASK_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 2U,
                         NULL);
    if (result != pdPASS)
    {
        application_error();
    }

    vTaskStartScheduler();
    application_error();
    return 0;
}

void vApplicationMallocFailedHook(void)
{
    application_error();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    application_error();
}
