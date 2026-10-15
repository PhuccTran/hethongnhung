# PWM LED và FreeRTOS Queue (STM32F103C8T6)

Chương trình sử dụng STM32F10x Standard Peripheral Library và FreeRTOS, theo cấu trúc dự án `tuan-4`.

## Kết nối phần cứng

- LED 1, 2, 3: lần lượt nối PA0/TIM2_CH1, PA1/TIM2_CH2 và PA2/TIM2_CH3; mỗi LED cần điện trở hạn dòng và nối về GND.
- Nút chọn LED: PB12 nối nút nhấn về GND.
- Nút tăng sáng: PB13 nối nút nhấn về GND.
- Nút giảm sáng: PB14 nối nút nhấn về GND.
- Các chân nút dùng pull-up nội, nên mức nhấn là LOW.

Các GPIO được chọn cho Blue Pill STM32F103C8T6, không dùng LED onboard PC13.

## Hoạt động

- TIM2 phát PWM phần cứng trên cả ba kênh với tần số 1 kHz.
- Có một task đọc ba nút, lấy mẫu mỗi 10 ms và debounce 30 ms.
- Mỗi lần nhấn nút chọn, LED được điều khiển chuyển vòng LED 1 → LED 2 → LED 3 → LED 1.
- Nút tăng/giảm thay đổi độ sáng LED đang chọn theo bước 10%, giới hạn từ 0% đến 100%.
- Mỗi LED có một task và một queue độ dài 1 riêng. Queue truyền trực tiếp độ sáng dạng phần trăm (0–100%). Task đọc nút chỉ gửi phần trăm của LED đang chọn vào queue tương ứng; task LED nhận phần trăm và tự đổi sang giá trị PWM cho kênh của mình. Queue dùng overwrite để giữ giá trị mới nhất nếu task LED chưa nhận kịp dữ liệu trước đó.

## Build và nạp

Dependencies được đặt riêng trong `vendor/` của bài này, không sửa hoặc phụ thuộc vào các bài tuần trước:

- FreeRTOS Kernel: [`FreeRTOS/FreeRTOS-Kernel`](https://github.com/FreeRTOS/FreeRTOS-Kernel), commit `8be86d4a24fd4091f8f4192018423ab590f408db`.
- STM32F10x SPL 3.5.0: [`shenyiqun/STM32F10x_StdPeriph_Lib_V3.5.0`](https://github.com/shenyiqun/STM32F10x_StdPeriph_Lib_V3.5.0), commit `5be388fd481f7d05cadccbf29426b31629513045`.

Các file nguồn cần thiết và giấy phép đi kèm đã được chép vào thư mục `vendor/`; không cần khởi tạo submodule để build.

```sh
make
make flash PORT=/dev/ttyUSB0 BAUD=115200
```

Lệnh flash dùng UART system bootloader: đặt BOOT0 = 1 và reset trước khi nạp, sau đó đặt BOOT0 = 0 và reset để chạy ứng dụng. Toolchain yêu cầu `arm-none-eabi-gcc`, `make` và `stm32flash`.
