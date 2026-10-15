#include <stdio.h>

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define BUTTON_PIN              GPIO_Pin_0
#define BUTTON_EXTI_LINE        EXTI_Line0
#define BUTTON_EXTI_IRQn        EXTI0_IRQn
#define BUTTON_DEBOUNCE_MS      30U
#define COUNT_QUEUE_LENGTH      10U
#define UART_TASK_STACK_SIZE    256U
#define UART_TASK_PRIORITY      (tskIDLE_PRIORITY + 1U)
#define UART_BAUDRATE           115200U

static QueueHandle_t count_queue;
static volatile uint32_t button_press_count;
static volatile uint8_t count_queue_overflowed;
static TickType_t last_press_tick;

static void application_error(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

static void clock_init(void)
{
    uint32_t timeout = 0x5000U;

    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    while ((RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET) && (timeout > 0U))
    {
        --timeout;
    }
    if (timeout == 0U)
    {
        application_error();
    }

    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);

    timeout = 0x5000U;
    while ((RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) && (timeout > 0U))
    {
        --timeout;
    }
    if (timeout == 0U)
    {
        application_error();
    }

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    timeout = 0x5000U;
    while ((RCC_GetSYSCLKSource() != 0x08U) && (timeout > 0U))
    {
        --timeout;
    }
    if (timeout == 0U)
    {
        application_error();
    }

    SystemCoreClockUpdate();
}

static void uart1_init(void)
{
    GPIO_InitTypeDef gpio_init;
    USART_InitTypeDef usart_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin = GPIO_Pin_10;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_init);

    usart_init.USART_BaudRate = UART_BAUDRATE;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Tx;
    USART_Init(USART1, &usart_init);
    USART_Cmd(USART1, ENABLE);
}

static void button_exti_init(void)
{
    GPIO_InitTypeDef gpio_init;
    EXTI_InitTypeDef exti_init;
    NVIC_InitTypeDef nvic_init;
    const TickType_t debounce_ticks = pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    gpio_init.GPIO_Pin = BUTTON_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio_init);

    last_press_tick = xTaskGetTickCount() - debounce_ticks;
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    EXTI_ClearITPendingBit(BUTTON_EXTI_LINE);
    exti_init.EXTI_Line = BUTTON_EXTI_LINE;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_init.EXTI_LineCmd = ENABLE;

    nvic_init.NVIC_IRQChannel = BUTTON_EXTI_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority =
        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;
    nvic_init.NVIC_IRQChannelSubPriority = 0U;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
    EXTI_Init(&exti_init);
}

static void uart1_send_string(const char *text)
{
    while (*text != '\0')
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        {
        }
        USART_SendData(USART1, (uint16_t)(uint8_t)*text);
        ++text;
    }
}

static void uart_task(void *argument)
{
    uint32_t count;
    char message[32];
    uint8_t overflow_reported = 0U;

    (void)argument;

    uart1_send_string("EXTI button counter ready\r\n");
    button_exti_init();

    for (;;)
    {
        if (xQueueReceive(count_queue, &count, portMAX_DELAY) == pdPASS)
        {
            if ((count_queue_overflowed != 0U) && (overflow_reported == 0U))
            {
                uart1_send_string("Warning: count queue overflowed\r\n");
                overflow_reported = 1U;
            }
            (void)snprintf(message, sizeof(message), "Button count: %lu\r\n",
                           (unsigned long)count);
            uart1_send_string(message);
        }
    }
}

void EXTI0_IRQHandler(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    BaseType_t send_result;
    TickType_t now;

    if (EXTI_GetITStatus(BUTTON_EXTI_LINE) != RESET)
    {
        uint32_t count;

        EXTI_ClearITPendingBit(BUTTON_EXTI_LINE);
        now = xTaskGetTickCountFromISR();

        if ((TickType_t)(now - last_press_tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS))
        {
            last_press_tick = now;
            count = ++button_press_count;
            send_result = xQueueSendFromISR(count_queue,
                                             &count,
                                             &higher_priority_task_woken);
            if (send_result != pdPASS)
            {
                count_queue_overflowed = 1U;
            }
            portYIELD_FROM_ISR(higher_priority_task_woken);
        }
    }
}

int main(void)
{
    BaseType_t result;

    clock_init();
    uart1_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    count_queue = xQueueCreate(COUNT_QUEUE_LENGTH, sizeof(uint32_t));
    if (count_queue == NULL)
    {
        application_error();
    }

    result = xTaskCreate(uart_task,
                         "UART_Tx",
                         UART_TASK_STACK_SIZE,
                         NULL,
                         UART_TASK_PRIORITY,
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
