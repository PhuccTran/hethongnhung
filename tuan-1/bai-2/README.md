# Bai 2 - STM32F103 bare-metal

Dieu khien 8 LED noi vao PA0-PA7. LED chay tu PA0 den PA7, sau do dao chieu
tu PA7 ve PA0 va lap lai.

Chuong trinh chi dung thanh ghi RCC va GPIOA, khong dung STD, HAL hay SPL.
Mac dinh LED active-high.

## Nạp firmware
Dùng OpenOCD:
make flash

Nếu máy bạn đã cài STLink tool và muốn dùng cách cũ:
make flash-stlink

## Video Demo
https://www.youtube.com/shorts/A8BPWwgAhS4

