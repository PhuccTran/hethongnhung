#define RCC_APB2ENR (*(volatile unsigned long *)0x40021018UL)  // Thanh ghi clock cho GPIOA
#define GPIOA_CRL   (*(volatile unsigned long *)0x40010800UL)  // Cấu hình mode cho PA0..PA7
#define GPIOA_ODR   (*(volatile unsigned long *)0x4001080CUL)  // Thanh ghi ghi trạng thái output của GPIOA

// hàm delay
static void delay(void)
{
    volatile unsigned long count;

    for (count = 0UL; count < 700000UL; ++count) {
        __asm volatile ("nop"); // clock mặc định 8M ==> 1 vòng lặp mất 1/8M = 0.125us => 700000 vòng lặp mất 87.5ms
    }
}

int main(void)
{
    unsigned long led = 0x01UL; // 00000001
    // trạng thái led đang chạy sang phải hay sang trái
    unsigned long moving_right = 1UL;

    // bật clock cho GPIOA
    RCC_APB2ENR |= (1UL << 2);

    /* Cấu hình PA0..PA7 ở chế   độ output push-pull */
    GPIOA_CRL = 0x22222222UL;

    while (1) {
        /*
         * Ghi giá trị led vào 8 bit thấp của GPIOA_ODR.
         * Các bit khác của ODR được giữ nguyên.
         * Ví dụ: led = 0x04 -> PA2 sáng.
         */
        GPIOA_ODR = (GPIOA_ODR & ~0xFFUL) | led;
        delay();

        if (moving_right != 0UL) {
            if (led == 0x80UL) { //10000000
                /* Đến cuối phải thì đổi hướng sang trái */
                moving_right = 0UL;
                led >>= 1;
            } else {
                /* Chạy sang phải */
                led <<= 1;
            }
        } else if (led == 0x01UL) {
            /* Đến cuối trái thì đổi hướng sang phải */
            moving_right = 1UL;
            led <<= 1;
        } else {
            /* Chạy sang trái */
            led >>= 1;
        }
    }
}