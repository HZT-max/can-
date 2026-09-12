#include <stdint.h>

#define RCC_AHBENR   (*(volatile uint32_t *)0x40021014U)
#define GPIOB_MODER  (*(volatile uint32_t *)0x48000400U)
#define GPIOB_ODR    (*(volatile uint32_t *)0x48000414U)

extern uint32_t _estack;
void Reset_Handler(void);
void Default_Handler(void);

typedef void (*vector_handler_t)(void);

__attribute__((section(".isr_vector")))
const vector_handler_t vector_table[] = {
    (vector_handler_t)&_estack,
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    0,
    0,
    0,
    0,
    Default_Handler,
    Default_Handler,
    0,
    Default_Handler,
    Default_Handler,
};

static void delay(void)
{
    for (volatile uint32_t i = 0; i < 700000U; i++) {
        __asm volatile ("nop");
    }
}

/*
 * 复位后直接使用默认HSI时钟，每次翻转PB1上的RUN灯。
 * 该程序只验证Flash启动、CPU执行和PB1硬件，不依赖操作系统。
 */
void Reset_Handler(void)
{
    RCC_AHBENR |= (1U << 18);       // 开启GPIOB时钟
    GPIOB_MODER &= ~(3U << 2);
    GPIOB_MODER |= (1U << 2);       // PB1设置为推挽输出

    while (1) {
        GPIOB_ODR ^= (1U << 1);     // 翻转RUN灯
        delay();
    }
}

void Default_Handler(void)
{
    while (1) {
    }
}
