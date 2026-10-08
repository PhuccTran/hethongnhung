#include "debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "stm32f10x.h"

#define DEBUG_USART_BAUDRATE 115200U

void debug_init(void)
{
    GPIO_InitTypeDef gpio_init;
    USART_InitTypeDef usart_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    usart_init.USART_BaudRate = DEBUG_USART_BAUDRATE;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Tx;
    USART_Init(USART1, &usart_init);
    USART_Cmd(USART1, ENABLE);
}

void debug_write(const char *data, size_t length)
{
    size_t index;

    for (index = 0U; index < length; ++index)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        {
        }
        USART_SendData(USART1, (uint16_t) data[index]);
    }

    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {
    }
}

void debug_printf(const char *format, ...)
{
    char buffer[160];
    va_list arguments;
    int length;

    va_start(arguments, format);
    length = vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);

    if (length > 0)
    {
        if ((size_t) length >= sizeof(buffer))
        {
            length = (int) sizeof(buffer) - 1;
        }
        debug_write(buffer, (size_t) length);
    }
}
