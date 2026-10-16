
/*
 * CHUONG TRINH DIEU KHIEN 3 LED BANG PWM
 * Vi dieu khien: STM32F103
 * RTOS: FreeRTOS
 *
 * PB12: Chon LED tiep theo
 * PB13: Tang do sang LED dang chon
 * PB14: Giam do sang LED dang chon
 *
 * PA0: TIM2_CH1 -> LED 1
 * PA1: TIM2_CH2 -> LED 2
 * PA2: TIM2_CH3 -> LED 3
 */
// Thu vien CMSIS va Standard Peripheral Library cua STM32F10x
#include "stm32f10x.h"
// Thu vien cot loi cua FreeRTOS
#include "FreeRTOS.h"
// Thu vien quan ly Queue (hang doi)
#include "queue.h"
// Thu vien quan ly Task (tac vu)
#include "task.h"
// So luong LED can dieu khien
#define LED_COUNT               3U
// Gia tri ARR cua timer: dem tu 0 den 999
// So dem trong mot chu ky PWM = 999 + 1 = 1000
#define PWM_PERIOD              999U

// Do sang toi da: 100%
#define BRIGHTNESS_MAX          100U

// Moi lan nhan nut tang/giam 10%
#define BRIGHTNESS_STEP         10U

// Chu ky doc trang thai nut nhan: 10 ms
#define BUTTON_SAMPLE_MS        10U

// Thoi gian chong doi nut nhan: 30 ms
#define BUTTON_DEBOUNCE_MS      30U

// Kich thuoc stack cua moi task LED
// Don vi thuc te phu thuoc port FreeRTOS;
// voi port ARM Cortex-M thong thuong, gia tri nay tinh theo word
#define LED_TASK_STACK_SIZE     256U

// Kich thuoc stack cua task doc nut nhan
#define BUTTON_TASK_STACK_SIZE  256U


// Cac nut nhan duoc noi vao GPIOB
#define BUTTON_SELECT_PIN GPIO_Pin_12  // PB12: chon LED
#define BUTTON_UP_PIN     GPIO_Pin_13  // PB13: tang do sang
#define BUTTON_DOWN_PIN   GPIO_Pin_14  // PB14: giam do sang


// Cau truc chua thong tin cau hinh cho moi task LED
typedef struct
{
    uint8_t channel;  // Kenh PWM: 0, 1 hoac 2

    // Queue nhan gia tri do sang tu task nut nhan
    QueueHandle_t brightness_queue;

} led_task_config_t;


// Cau truc luu trang thai chong doi cho mot nut nhan
typedef struct
{
    // Trang thai da on dinh sau khi chong doi
    // 0 = nha nut, 1 = nhan nut
    uint8_t stable_pressed;

    // Trang thai dang duoc kiem tra
    uint8_t candidate_pressed;

    // Thoi diem bat dau xuat hien trang thai dang kiem tra
    TickType_t candidate_since;

} button_state_t;


// Mang chua Queue cua 3 LED
static QueueHandle_t led_queues[LED_COUNT];

// Mang cau hinh cho 3 task LED
static led_task_config_t led_configs[LED_COUNT];


// Ham xu ly loi nghiem trong
static void application_error(void)
{
    // Tat ngat toan cuc theo co che cua FreeRTOS
    taskDISABLE_INTERRUPTS();

    // Dung chuong trinh tai day
    for (;;)
    {
        // Khong lam gi nua
    }
}


// Khoi tao PWM tren TIM2
static void pwm_init(void)
{
    GPIO_InitTypeDef gpio_init;
    TIM_TimeBaseInitTypeDef timer_base;
    TIM_OCInitTypeDef timer_output;

    // Bat clock cho GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // Bat clock cho bo dinh thoi TIM2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);


    // Chon cac chan PA0, PA1, PA2
    gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;

    // Cau hinh chan o che do Alternate Function Push-Pull
    // De ngoai vi timer dieu khien tin hieu tren chan GPIO
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;

    // Toc do GPIO toi da duoc chon la 50 MHz
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;

    // Ap dung cau hinh cho GPIOA
    GPIO_Init(GPIOA, &gpio_init);


    // Cau hinh bo dem thoi gian TIM2

    // Chia tan so clock timer cho 72:
    // PSC = 71 -> he so chia = PSC + 1 = 72
    timer_base.TIM_Prescaler = 71U;

    // Timer dem len: 0, 1, 2, ... ARR
    timer_base.TIM_CounterMode = TIM_CounterMode_Up;

    // Gia tri dem toi da la 999
    timer_base.TIM_Period = PWM_PERIOD;

    // Khong chia them clock cho bo loc dau vao timer
    timer_base.TIM_ClockDivision = TIM_CKD_DIV1;

    // TIM2 khong su dung bo dem lap lai nhu timer nang cao
    timer_base.TIM_RepetitionCounter = 0U;

    // Nap cau hinh vao TIM2
    TIM_TimeBaseInit(TIM2, &timer_base);


    // Cau hinh chung cho cac kenh output compare

    // PWM mode 1:
    // Dau ra o muc active khi CNT < CCR
    timer_output.TIM_OCMode = TIM_OCMode_PWM1;

    // Cho phep dau ra cua kenh PWM
    timer_output.TIM_OutputState = TIM_OutputState_Enable;

    // Ban dau CCR = 0 -> duty cycle bang 0%
    timer_output.TIM_Pulse = 0U;

    // Tin hieu PWM active o muc cao
    timer_output.TIM_OCPolarity = TIM_OCPolarity_High;


    // Khoi tao kenh PWM 1: TIM2_CH1 tren PA0
    TIM_OC1Init(TIM2, &timer_output);

    // Khoi tao kenh PWM 2: TIM2_CH2 tren PA1
    TIM_OC2Init(TIM2, &timer_output);

    // Khoi tao kenh PWM 3: TIM2_CH3 tren PA2
    TIM_OC3Init(TIM2, &timer_output);


    // Bat preload cho thanh ghi so sanh CCR1
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

    // Bat preload cho thanh ghi so sanh CCR2
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);

    // Bat preload cho thanh ghi so sanh CCR3
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

    // Bat preload cho thanh ghi ARR
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    // Bat dau cho TIM2 dem va phat PWM
    TIM_Cmd(TIM2, ENABLE);
}


// Khoi tao cac nut nhan
static void buttons_init(void)
{
    GPIO_InitTypeDef gpio_init;

    // Bat clock cho GPIOB
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // Chon PB12, PB13, PB14
    gpio_init.GPIO_Pin =
        BUTTON_SELECT_PIN |
        BUTTON_UP_PIN |
        BUTTON_DOWN_PIN;

    // Bat dien tro keo len noi (pull-up)
    // Nha nut: doc duoc muc 1
    // Nhan nut noi xuong GND: doc duoc muc 0
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;

    // Ap dung cau hinh cho GPIOB
    GPIO_Init(GPIOB, &gpio_init);
}


// Chuyen do sang phan tram thanh gia tri CCR cua PWM
static uint16_t brightness_to_pulse(uint8_t brightness_percent)
{
    // Cong thuc:
    // CCR = brightness_percent * (ARR + 1) / 100
    //
    // Ep sang uint32_t truoc khi nhan de tranh tran so
    // voi kieu uint8_t.
    // Ep ve uint16_t vi CCR co do rong 16 bit.
    return (uint16_t)(
        ((uint32_t)brightness_percent * (PWM_PERIOD + 1U)) / 100U
    );
}


// Gui muc do sang hien tai vao Queue cua LED duoc chon
static void publish_selected_brightness(
    uint8_t selected_led,
    const uint8_t brightness[LED_COUNT])
{
    // Lay do sang cua LED dang duoc chon
    uint8_t brightness_percent = brightness[selected_led];

    // Ghi de gia tri cu bang gia tri moi trong Queue.
    // Queue phai co do dai 1 phan tu.
    // Phu hop khi chi can giu muc do sang moi nhat.
    if (xQueueOverwrite(
            led_queues[selected_led],
            &brightness_percent) != pdPASS)
    {
        // Neu thao tac that bai thi dung tai ham xu ly loi
        application_error();
    }
}


// Task dieu khien mot LED
// Moi LED co mot task rieng
static void led_task(void *argument)
{
    // argument tro den cau hinh cua LED tuong ung
    const led_task_config_t *config =
        (const led_task_config_t *)argument;

    // Bien nhan gia tri do sang tu Queue
    uint8_t brightness_percent;

    // Task chay lien tuc cho den khi he thong dung
    for (;;)
    {
        // Cho den khi Queue co du lieu.
        // portMAX_DELAY: cho vo han neu cau hinh FreeRTOS
        // cho phep block vo han.
        if (xQueueReceive(
                config->brightness_queue,
                &brightness_percent,
                portMAX_DELAY) == pdPASS)
        {
            // Chuyen phan tram do sang sang gia tri CCR
            uint16_t pulse =
                brightness_to_pulse(brightness_percent);

            // Chon kenh PWM tuong ung voi LED
            switch (config->channel)
            {
                case 0U:
                    // Cap nhat do rong xung cua kenh 1
                    TIM_SetCompare1(TIM2, pulse);
                    break;

                case 1U:
                    // Cap nhat do rong xung cua kenh 2
                    TIM_SetCompare2(TIM2, pulse);
                    break;

                case 2U:
                    // Cap nhat do rong xung cua kenh 3
                    TIM_SetCompare3(TIM2, pulse);
                    break;

                default:
                    // Kenh khong hop le
                    application_error();
                    break;
            }
        }
    }
}


// Doc trang thai nut nhan
static uint8_t button_is_pressed(uint16_t pin)
{
    // Do dung dien tro pull-up:
    // Bit_RESET (0) = nut dang nhan
    // Bit_SET   (1) = nut dang nha
    //
    // Tra ve 1 neu nhan, 0 neu nha
    return GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET
           ? 1U : 0U;
}


// Xu ly su kien nhan nut
static void handle_button_press(
    uint8_t button_index,
    uint8_t *selected_led,
    uint8_t brightness[LED_COUNT])
{
    // Nut thu 0: chuyen sang LED tiep theo
    if (button_index == 0U)
    {
        // Chuyen LED 0 -> 1 -> 2 -> 0 -> ...
        *selected_led =
            (uint8_t)((*selected_led + 1U) % LED_COUNT);
    }

    // Nut thu 1: tang do sang
    else if (button_index == 1U)
    {
        // Chi tang neu do sang chua dat 100%
        if (brightness[*selected_led] < BRIGHTNESS_MAX)
        {
            // Tang them 10%
            brightness[*selected_led] =
                (uint8_t)(
                    brightness[*selected_led] + BRIGHTNESS_STEP
                );
        }
    }

    // Nut thu 2: giam do sang
    else
    {
        // Chi giam neu do sang lon hon 0%
        if (brightness[*selected_led] > 0U)
        {
            // Giam them 10%
            brightness[*selected_led] =
                (uint8_t)(
                    brightness[*selected_led] - BRIGHTNESS_STEP
                );
        }
    }

    // Gui muc do sang hien tai cua LED dang chon
    // vao Queue de task LED cap nhat PWM.
    //
    // Khi chi chuyen LED, ham nay van gui lai muc sang
    // hien tai cua LED moi duoc chon.
    publish_selected_brightness(*selected_led, brightness);
}


// Task doc va xu ly ba nut nhan
static void buttons_task(void *argument)
{
    // Mang chua cac chan nut theo thu tu:
    // index 0 = SELECT, index 1 = UP, index 2 = DOWN
    static const uint16_t button_pins[] = {
        BUTTON_SELECT_PIN,
        BUTTON_UP_PIN,
        BUTTON_DOWN_PIN
    };

    // Luu trang thai chong doi cua tung nut
    button_state_t states[3];

    // Do sang ban dau cua ca ba LED deu bang 0%
    uint8_t brightness[LED_COUNT] = {0U, 0U, 0U};

    // Ban dau chon LED dau tien: index 0
    uint8_t selected_led = 0U;

    // Bien lap qua ba nut
    uint8_t button;

    // Bien luu tick hien tai cua FreeRTOS
    TickType_t now;

    // Chuyen 30 ms sang so tick cua he thong
    const TickType_t debounce_ticks =
        pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS);

    // Tham so argument khong duoc su dung
    (void)argument;


    // Doc thoi diem hien tai
    now = xTaskGetTickCount();

    // Khoi tao trang thai ban dau cua tung nut
    for (button = 0U; button < 3U; ++button)
    {
        // Doc trang thai thuc te cua nut
        uint8_t pressed = button_is_pressed(button_pins[button]);

        // Xem trang thai hien tai la trang thai on dinh ban dau
        states[button].stable_pressed = pressed;

        // Trang thai dang kiem tra cung bang trang thai hien tai
        states[button].candidate_pressed = pressed;

        // Ghi lai thoi diem bat dau kiem tra
        states[button].candidate_since = now;
    }


    // Vong lap chinh cua task nut nhan
    for (;;)
    {
        // Cap nhat tick hien tai
        now = xTaskGetTickCount();

        // Kiem tra lan luot tung nut
        for (button = 0U; button < 3U; ++button)
        {
            // Doc trang thai hien tai cua nut
            uint8_t pressed =
                button_is_pressed(button_pins[button]);


            // Neu trang thai doc duoc khac trang thai dang kiem tra
            if (pressed != states[button].candidate_pressed)
            {
                // Cap nhat trang thai dang kiem tra
                states[button].candidate_pressed = pressed;

                // Bat dau tinh lai thoi gian chong doi
                states[button].candidate_since = now;
            }

            // Neu trang thai khong thay doi trong thoi gian du dai
            // va khac voi trang thai on dinh truoc do
            else if (
                (pressed != states[button].stable_pressed) &&
                ((TickType_t)(now - states[button].candidate_since)
                    >= debounce_ticks)
            )
            {
                // Chap nhan trang thai moi la trang thai on dinh
                states[button].stable_pressed = pressed;

                // Chi xu ly khi nut chuyen sang trang thai nhan.
                // Khong xu ly lai khi nut duoc nha ra.
                if (pressed != 0U)
                {
                    // Thuc hien chuc nang cua nut vua nhan
                    handle_button_press(
                        button,
                        &selected_led,
                        brightness
                    );
                }
            }
        }

        // Tam dung task trong 10 ms truoc khi doc tiep.
        // Task khac van co the duoc CPU xu ly trong luc nay.
        vTaskDelay(pdMS_TO_TICKS(BUTTON_SAMPLE_MS));
    }
}


// Ham main: diem bat dau cua chuong trinh
int main(void)
{
    // Bien lap qua cac LED
    uint8_t led;

    // Nhan ket qua tao task
    BaseType_t result;


    // Khoi tao ngoai vi PWM
    pwm_init();

    // Khoi tao cac nut nhan
    buttons_init();


    // Tao Queue va task rieng cho tung LED
    for (led = 0U; led < LED_COUNT; ++led)
    {
        // Tao Queue co do dai 1, moi phan tu la uint8_t.
        // Queue chua phan tram do sang tu 0 den 100.
        led_queues[led] = xQueueCreate(
            1U,
            sizeof(uint8_t)
        );

        // Neu khong tao duoc Queue thi dung chuong trinh
        if (led_queues[led] == NULL)
        {
            application_error();
        }


        // Luu so thu tu kenh PWM cho LED
        led_configs[led].channel = led;

        // Gan Queue tuong ung vao cau hinh LED
        led_configs[led].brightness_queue = led_queues[led];


        // Tao task dieu khien LED
        result = xTaskCreate(
            led_task,

            // Dat ten task theo LED
            led == 0U ? "LED_1" :
            (led == 1U ? "LED_2" : "LED_3"),

            // Kich thuoc stack
            LED_TASK_STACK_SIZE,

            // Truyen dia chi cau hinh cua LED cho task
            &led_configs[led],

            // Uu tien = 1, cao hon task IDLE
            tskIDLE_PRIORITY + 1U,

            // Khong luu TaskHandle
            NULL
        );

        // Neu tao task that bai thi dung chuong trinh
        if (result != pdPASS)
        {
            application_error();
        }
    }


    // Tao task doc nut nhan
    result = xTaskCreate(
        buttons_task,

        // Ten task
        "Buttons",

        // Kich thuoc stack
        BUTTON_TASK_STACK_SIZE,

        // Khong can tham so truyen vao
        NULL,

        // Uu tien cao hon cac task LED
        tskIDLE_PRIORITY + 2U,

        // Khong luu TaskHandle
        NULL
    );

    // Kiem tra viec tao task nut nhan
    if (result != pdPASS)
    {
        application_error();
    }


    // Bat dau bo lap lich FreeRTOS.
    // Tu day, cac task duoc lap lich de thuc thi.
    vTaskStartScheduler();

    // Neu ham nay tra ve thi scheduler khong khoi dong duoc
    // (vi du khong du bo nho heap).
    application_error();

    return 0;
}


// Hook duoc goi khi FreeRTOS khong cap phat duoc bo nho dong
void vApplicationMallocFailedHook(void)
{
    // Dung he thong de tranh tiep tuc chay trong trang thai loi
    application_error();
}


// Hook duoc goi khi phat hien tran stack cua task
void vApplicationStackOverflowHook(
    TaskHandle_t task,
    char *task_name)
{
    // Hai tham so nay duoc cung cap de debug ten task bi loi
    // Trong chuong trinh nay chua su dung chung
    (void)task;
    (void)task_name;

    // Dung he thong khi phat hien tran stack
    application_error();
}
