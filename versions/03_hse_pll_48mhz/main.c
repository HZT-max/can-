#include <stdint.h>

#define RCC_CR       (*(volatile uint32_t *)0x40021000U)
#define RCC_CFGR     (*(volatile uint32_t *)0x40021004U)
#define RCC_AHBENR   (*(volatile uint32_t *)0x40021014U)
#define RCC_CFGR2    (*(volatile uint32_t *)0x4002102CU)
#define FLASH_ACR    (*(volatile uint32_t *)0x40022000U)
#define GPIOB_MODER  (*(volatile uint32_t *)0x48000400U)
#define GPIOB_ODR    (*(volatile uint32_t *)0x48000414U)

#define RCC_CR_HSEON       (1U << 16)
#define RCC_CR_HSERDY      (1U << 17)
#define RCC_CR_PLLON       (1U << 24)
#define RCC_CR_PLLRDY      (1U << 25)
#define RCC_CFGR_PLLSRC_HSE (2U << 15)
#define RCC_CFGR_PLLMUL6   (4U << 18)
#define RCC_CFGR_SW_PLL    2U
#define RCC_CFGR_SWS_PLL   (2U << 2)
#define FLASH_ACR_48MHZ    0x11U

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
    for (volatile uint32_t i = 0; i < 4200000U; i++) {
        __asm volatile ("nop");
    }
}

/*
 * 使用板上的8 MHz HSE，经PLL倍频6得到48 MHz系统时钟。
 * 返回0表示晶振、PLL或时钟切换没有在超时前就绪。
 */
static uint32_t clock_init_48mhz(void)
{
    uint32_t timeout = 1000000U;

    RCC_CR |= RCC_CR_HSEON;
    while ((RCC_CR & RCC_CR_HSERDY) == 0U && --timeout != 0U) {
    }
    if (timeout == 0U) {
        return 0U;
    }

    RCC_CFGR2 &= ~0xFU; // HSE预分频保持为1
    RCC_CFGR &= ~((3U << 15) | (0xFU << 18));
    RCC_CFGR |= RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMUL6;

    RCC_CR |= RCC_CR_PLLON;
    timeout = 1000000U;
    while ((RCC_CR & RCC_CR_PLLRDY) == 0U && --timeout != 0U) {
    }
    if (timeout == 0U) {
        return 0U;
    }

    FLASH_ACR = FLASH_ACR_48MHZ; // 预取使能，Flash等待周期设为1
    RCC_CFGR = (RCC_CFGR & ~3U) | RCC_CFGR_SW_PLL;

    timeout = 1000000U;
    while ((RCC_CFGR & (3U << 2)) != RCC_CFGR_SWS_PLL && --timeout != 0U) {
    }
    return timeout != 0U;
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

    if (clock_init_48mhz() == 0U) {
        GPIOB_ODR |= (1U << 1);     // HSE或PLL失败时RUN灯常亮
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
