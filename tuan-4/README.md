# STM32F103C8T6 + FreeRTOS + SPL

Project mẫu cho Blue Pill STM32F103C8T6, dùng STM32F10x Standard Peripheral Library và FreeRTOS Kernel.

## Build

Yêu cầu:

- `arm-none-eabi-gcc`
- `make`
- `stm32flash` nếu muốn nạp qua UART bootloader

```sh
make
```

File sinh ra trong `build/` gồm `.elf`, `.hex`, `.bin` và `.map`.

## Flash qua UART

```sh
make flash PORT=/dev/ttyUSB0 BAUD=115200
```

Trước khi chạy lệnh, nối UART với mức 3.3 V, đặt `BOOT0 = 1`, reset board để vào system bootloader, rồi chạy lệnh. Sau khi nạp xong, đặt `BOOT0 = 0` và reset lại board.

Ứng dụng mặc định nhấp nháy LED onboard tại PC13 mỗi 500 ms. HSE được cấu hình 8 MHz và hệ thống chạy 72 MHz.

## Cấu trúc

- `app/`: application và cấu hình SPL/FreeRTOS
- `linker/`: linker script cho 64 KB flash, 20 KB RAM
- `vendor/STM32F10x_StdPeriph_Lib/`: STM32F10x SPL
- `vendor/FreeRTOS-Kernel/`: FreeRTOS Kernel
