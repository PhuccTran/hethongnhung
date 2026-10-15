# Bộ đếm nút nhấn EXTI với Queue của FreeRTOS (STM32F103C8T6)
Dự án sử dụng thư viện STM32F10x Standard Peripheral Library và FreeRTOS

## 1. Kết nối phần cứng

- Nút nhấn: Nối chân PA0 với GND. Chân PA0 sử dụng điện trở kéo lên nội (internal pull-up). Khi nhấn nút, tín hiệu tạo ra một cạnh xuống (falling edge) kích hoạt ngắt EXTI0.
- Bộ chuyển đổi USB-to-UART: Nối chân PA9 (USART1_TX) với chân RX của bộ chuyển đổi, đồng thời nối GND của bo mạch với GND của bộ chuyển đổi.
- Cấu hình Serial Terminal: Tốc độ truyền 115200 baud, 8 bit dữ liệu, không có bit chẵn lẻ (no parity), 1 bit dừng (1 stop bit), tức cấu hình 8-N-1.
- Firmware giả định bo mạch sử dụng thạch anh HSE 8 MHz, giống như trên bo mạch Blue Pill, và cấu hình xung nhịp hệ thống lên 72 MHz.



## Nguyên lý hoạt động

-Ngắt EXTI0 được cấu hình kích hoạt ở cạnh xuống (falling edge), với mức ưu tiên NVIC là 5. Mức ưu tiên này cho phép ngắt gọi các API FreeRTOS có hậu tố ...FromISR theo cấu hình được cung cấp.
- Mỗi khi phát hiện một cạnh nhấn nút hợp lệ, bộ đếm sẽ tăng thêm 1 và gửi giá trị 32 bit của bộ đếm vào hàng đợi (Queue) của FreeRTOS bằng hàm xQueueSendFromISR.
- Cơ chế chống dội phím (debounce) sử dụng tick của ISR với khoảng thời gian 30 ms để loại bỏ các xung nhiễu do tiếp điểm nút nhấn bị nảy.
- Hàng đợi có sức chứa 10 phần tử. Nếu có một chuỗi sự kiện đến quá nhanh khiến hàng đợi bị đầy, tác vụ UART sẽ thông báo cảnh báo tràn hàng đợi (queue overflow).
- Tác vụ UART chờ nhận dữ liệu bằng hàm xQueueReceive(..., portMAX_DELAY). Khi nhận được giá trị bộ đếm, tác vụ định dạng và gửi chuỗi Button count: N qua USART1.
- Hàm portYIELD_FROM_ISR yêu cầu chuyển đổi ngữ cảnh ngay lập tức nếu tác vụ đang bị chặn được đánh thức và chuyển sang trạng thái sẵn sàng chạy.

## Build and flash

```sh
make
make flash PORT=/dev/ttyUSB0 BAUD=115200
```
