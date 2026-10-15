# EXTI button counter with FreeRTOS Queue (STM32F103C8T6)

The project uses STM32F10x Standard Peripheral Library and FreeRTOS. All dependency files are local to this exercise under `vendor/`; it does not modify or rely on earlier weeks.

## Wiring

- Push button: PA0 to GND. PA0 uses its internal pull-up; pressing the button creates a falling edge on EXTI0.
- USB-to-UART adapter: PA9 (USART1_TX) to adapter RX, and board GND to adapter GND.
- Serial terminal: 115200 baud, 8 data bits, no parity, 1 stop bit (8-N-1).
- The firmware assumes an 8 MHz HSE crystal, as on a Blue Pill board, and configures the system clock to 72 MHz.

## Operation

- EXTI0 is configured for a falling edge and runs at NVIC priority 5, which is permitted to call FreeRTOS `...FromISR` APIs with the supplied configuration.
- Each accepted button edge increments the counter and sends its 32-bit value to a FreeRTOS queue using `xQueueSendFromISR`.
- A 30 ms ISR tick-based debounce rejects switch bounce. The queue has 10 entries; if an unusually fast burst fills it, the UART task reports a queue-overflow warning.
- The UART task blocks in `xQueueReceive(..., portMAX_DELAY)`. When a count arrives, it formats and sends `Button count: N` through USART1. `portYIELD_FROM_ISR` requests an immediate context switch when the blocked task is made ready.

## Build and flash

```sh
make
make flash PORT=/dev/ttyUSB0 BAUD=115200
```

The UART flash command is for the STM32 system bootloader: set BOOT0 high and reset before flashing; set BOOT0 low and reset afterwards. Open the serial terminal after the application starts.
