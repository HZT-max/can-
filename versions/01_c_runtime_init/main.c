#include <stdint.h>

#define RCC_AHBENR   (*(volatile uint32_t *)0x40021014U)
#define GPIOB_MODER  (*(volatile uint32_t *)0x48000400U)
#define GPIOB_ODR    (*(volatile uint32_t *)0x48000414U)

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
void Reset_Handler(void);
void Default_Handler(void);

volatile uint32_t initialized_marker = 0x13579BDFU;
volatile uint32_t zero_marker;

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
 * 在原始闪灯基准上只增加标准C运行时初始化：复制.data并清零.bss。
 * 标记变量检查通过后仍执行同一段PB1闪灯逻辑。
 */
void Reset_Handler(void)
{
    uint32_t *source = &_sidata;
    for (uint32_t *destination = &_sdata; destination < &_edata; destination++) {
        *destination = *source++;
    }

    for (uint32_t *destination = &_sbss; destination < &_ebss; destination++) {
        *destination = 0;
    }

    RCC_AHBENR |= (1U << 18);       // 开启GPIOB时钟
    GPIOB_MODER &= ~(3U << 2);
    GPIOB_MODER |= (1U << 2);       // PB1设置为推挽输出

    if (initialized_marker != 0x13579BDFU || zero_marker != 0U) {
        GPIOB_ODR |= (1U << 1);     // 初始化失败时RUN灯常亮，不再继续
        while (1) {
        }
    }

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
