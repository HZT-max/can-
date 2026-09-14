#include <stdint.h>

#define RCC_CR       (*(volatile uint32_t *)0x40021000U)
#define RCC_CFGR     (*(volatile uint32_t *)0x40021004U)
#define RCC_APB2RSTR (*(volatile uint32_t *)0x4002100CU)
#define RCC_APB1RSTR (*(volatile uint32_t *)0x40021010U)
#define RCC_AHBENR   (*(volatile uint32_t *)0x40021014U)
#define RCC_APB2ENR  (*(volatile uint32_t *)0x40021018U)
#define RCC_APB1ENR  (*(volatile uint32_t *)0x4002101CU)
#define RCC_CFGR2    (*(volatile uint32_t *)0x4002102CU)
#define FLASH_ACR    (*(volatile uint32_t *)0x40022000U)
#define GPIOA_MODER  (*(volatile uint32_t *)0x48000000U)
#define GPIOA_OTYPER (*(volatile uint32_t *)0x48000004U)
#define GPIOA_OSPEEDR (*(volatile uint32_t *)0x48000008U)
#define GPIOA_PUPDR  (*(volatile uint32_t *)0x4800000CU)
#define GPIOA_AFRL   (*(volatile uint32_t *)0x48000020U)
#define GPIOA_AFRH   (*(volatile uint32_t *)0x48000024U)
#define GPIOB_MODER  (*(volatile uint32_t *)0x48000400U)
#define GPIOB_OTYPER (*(volatile uint32_t *)0x48000404U)
#define GPIOB_OSPEEDR (*(volatile uint32_t *)0x48000408U)
#define GPIOB_PUPDR  (*(volatile uint32_t *)0x4800040CU)
#define GPIOB_ODR    (*(volatile uint32_t *)0x48000414U)
#define GPIOB_AFRH   (*(volatile uint32_t *)0x48000424U)
#define SYST_CSR     (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR     (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR     (*(volatile uint32_t *)0xE000E018U)

#define RCC_CR_HSEON       (1U << 16)
#define RCC_CR_HSERDY      (1U << 17)
#define RCC_CR_PLLON       (1U << 24)
#define RCC_CR_PLLRDY      (1U << 25)
#define RCC_CFGR_PLLSRC_HSE (2U << 15)
#define RCC_CFGR_PLLMUL6   (4U << 18)
#define RCC_CFGR_SW_PLL    2U
#define RCC_CFGR_SWS_PLL   (2U << 2)
#define FLASH_ACR_48MHZ    0x11U
#define SYST_CSR_ENABLE    (1U << 0)
#define SYST_CSR_TICKINT   (1U << 1)
#define SYST_CSR_CLKSOURCE (1U << 2)

#define RCC_AHBENR_GPIOAEN   (1U << 17)
#define RCC_AHBENR_GPIOBEN   (1U << 18)
#define RCC_APB2_USART1      (1U << 14)
#define RCC_APB1_USART2      (1U << 17)
#define RCC_APB1_USART3      (1U << 18)
#define RCC_APB1_USART4      (1U << 19)
#define RCC_APB1_CAN         (1U << 25)
#define USART_CR1_ENABLE_RX_TX 0x0DU
#define UART_BRR_115200       417U

#define CAN_MCR              (*(volatile uint32_t *)0x40006400U)
#define CAN_MSR              (*(volatile uint32_t *)0x40006404U)
#define CAN_BTR              (*(volatile uint32_t *)0x4000641CU)
#define CAN_MCR_INRQ         (1U << 0)
#define CAN_MCR_SLEEP        (1U << 1)
#define CAN_MCR_AWUM         (1U << 5)
#define CAN_MCR_ABOM         (1U << 6)
#define CAN_MSR_INAK         (1U << 0)
#define CAN_BTR_1MBIT_48MHZ  0x00050005U
#define CAN_BTR_LBKM         (1U << 30)
#define CAN_BTR_SILM         (1U << 31)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t GTPR;
    volatile uint32_t RTOR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} USARTRegisters;

#define USART1_REGS ((USARTRegisters *)0x40013800U)
#define USART2_REGS ((USARTRegisters *)0x40004400U)
#define USART3_REGS ((USARTRegisters *)0x40004800U)
#define USART4_REGS ((USARTRegisters *)0x40004C00U)

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);

volatile uint32_t initialized_marker = 0x13579BDFU;
volatile uint32_t zero_marker;
static volatile uint32_t systick_millis;

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
    SysTick_Handler,
};

/* SysTick每1 ms进入一次，用于证明中断向量和48 MHz时基正确。 */
void SysTick_Handler(void)
{
    systick_millis++;
}

/* 异常时保持RUN灯熄灭，用于和正常的1 s翻转明确区分。 */
static void stop_with_led_off(void)
{
    GPIOB_ODR &= ~(1U << 1);
    while (1) {
    }
}

static void gpio_set_alternate(volatile uint32_t *moder,
                               volatile uint32_t *otyper,
                               volatile uint32_t *ospeedr,
                               volatile uint32_t *pupdr,
                               volatile uint32_t *afr,
                               uint8_t pin,
                               uint8_t af)
{
    const uint8_t local_pin = pin & 7U;
    *moder = (*moder & ~(3U << (pin * 2U))) | (2U << (pin * 2U));
    *otyper &= ~(1U << pin);
    *ospeedr |= 3U << (pin * 2U);
    *pupdr &= ~(3U << (pin * 2U));
    *afr = (*afr & ~(0xFU << (local_pin * 4U))) | ((uint32_t)af << (local_pin * 4U));
}

static void uart_enable(USARTRegisters *uart)
{
    uart->CR1 = 0U;
    uart->CR2 = 0U;
    uart->CR3 = 0U;
    uart->BRR = UART_BRR_115200;
    uart->ICR = 0xFFFFFFFFU;
    uart->CR1 = USART_CR1_ENABLE_RX_TX;
}

/* 只配置四路UART引脚、时钟和115200波特率，本阶段不收发数据。 */
static uint32_t uart4_init(void)
{
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

    gpio_set_alternate(&GPIOA_MODER, &GPIOA_OTYPER, &GPIOA_OSPEEDR, &GPIOA_PUPDR,
                       &GPIOA_AFRH, 9U, 1U);   // USART1 TX PA9 AF1
    gpio_set_alternate(&GPIOA_MODER, &GPIOA_OTYPER, &GPIOA_OSPEEDR, &GPIOA_PUPDR,
                       &GPIOA_AFRH, 10U, 1U);  // USART1 RX PA10 AF1
    gpio_set_alternate(&GPIOA_MODER, &GPIOA_OTYPER, &GPIOA_OSPEEDR, &GPIOA_PUPDR,
                       &GPIOA_AFRL, 2U, 1U);   // USART2 TX PA2 AF1
    gpio_set_alternate(&GPIOA_MODER, &GPIOA_OTYPER, &GPIOA_OSPEEDR, &GPIOA_PUPDR,
                       &GPIOA_AFRL, 3U, 1U);   // USART2 RX PA3 AF1
    gpio_set_alternate(&GPIOB_MODER, &GPIOB_OTYPER, &GPIOB_OSPEEDR, &GPIOB_PUPDR,
                       &GPIOB_AFRH, 10U, 4U);  // USART3 TX PB10 AF4
    gpio_set_alternate(&GPIOB_MODER, &GPIOB_OTYPER, &GPIOB_OSPEEDR, &GPIOB_PUPDR,
                       &GPIOB_AFRH, 11U, 4U);  // USART3 RX PB11 AF4
    gpio_set_alternate(&GPIOA_MODER, &GPIOA_OTYPER, &GPIOA_OSPEEDR, &GPIOA_PUPDR,
                       &GPIOA_AFRL, 0U, 4U);   // USART4 TX PA0 AF4
    gpio_set_alternate(&GPIOA_MODER, &GPIOA_OTYPER, &GPIOA_OSPEEDR, &GPIOA_PUPDR,
                       &GPIOA_AFRL, 1U, 4U);   // USART4 RX PA1 AF4

    RCC_APB2ENR |= RCC_APB2_USART1;
    RCC_APB1ENR |= RCC_APB1_USART2 | RCC_APB1_USART3 | RCC_APB1_USART4;
    RCC_APB2RSTR |= RCC_APB2_USART1;
    RCC_APB2RSTR &= ~RCC_APB2_USART1;
    RCC_APB1RSTR |= RCC_APB1_USART2 | RCC_APB1_USART3 | RCC_APB1_USART4;
    RCC_APB1RSTR &= ~(RCC_APB1_USART2 | RCC_APB1_USART3 | RCC_APB1_USART4);

    uart_enable(USART1_REGS);
    uart_enable(USART2_REGS);
    uart_enable(USART3_REGS);
    uart_enable(USART4_REGS);

    return (USART1_REGS->BRR == UART_BRR_115200) &&
           (USART2_REGS->BRR == UART_BRR_115200) &&
           (USART3_REGS->BRR == UART_BRR_115200) &&
           (USART4_REGS->BRR == UART_BRR_115200) &&
           (USART1_REGS->CR1 == USART_CR1_ENABLE_RX_TX) &&
           (USART2_REGS->CR1 == USART_CR1_ENABLE_RX_TX) &&
           (USART3_REGS->CR1 == USART_CR1_ENABLE_RX_TX) &&
           (USART4_REGS->CR1 == USART_CR1_ENABLE_RX_TX);
}

/*
 * 验证bxCAN进入初始化模式、启用静默内部回环并退出初始化模式。
 * 内部回环隔离外部收发器和CANH/CANL，用于判断MCU内部CAN是否能稳定启动。
 */
static uint32_t bxcan_enter_and_exit_init(void)
{
    gpio_set_alternate(&GPIOB_MODER, &GPIOB_OTYPER, &GPIOB_OSPEEDR, &GPIOB_PUPDR,
                       &GPIOB_AFRH, 8U, 4U);   // CAN RX PB8 AF4
    gpio_set_alternate(&GPIOB_MODER, &GPIOB_OTYPER, &GPIOB_OSPEEDR, &GPIOB_PUPDR,
                       &GPIOB_AFRH, 9U, 4U);   // CAN TX PB9 AF4

    RCC_APB1ENR |= RCC_APB1_CAN;
    RCC_APB1RSTR |= RCC_APB1_CAN;
    RCC_APB1RSTR &= ~RCC_APB1_CAN;

    CAN_MCR &= ~CAN_MCR_SLEEP;
    CAN_MCR |= CAN_MCR_INRQ;

    const uint32_t start_ms = systick_millis;
    while ((CAN_MSR & CAN_MSR_INAK) == 0U) {
        if ((uint32_t)(systick_millis - start_ms) >= 100U) {
            return 0U;
        }
    }

    CAN_MCR = CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_INRQ;
    CAN_BTR = CAN_BTR_1MBIT_48MHZ | CAN_BTR_LBKM | CAN_BTR_SILM;
    CAN_MCR &= ~CAN_MCR_INRQ;

    const uint32_t exit_start_ms = systick_millis;
    while ((CAN_MSR & CAN_MSR_INAK) != 0U) {
        if ((uint32_t)(systick_millis - exit_start_ms) >= 1000U) {
            return 0U;
        }
    }
    return 1U;
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
    GPIOB_ODR &= ~(1U << 1);        // 异常统一表现为RUN灯不亮

    if (initialized_marker != 0x13579BDFU || zero_marker != 0U) {
        stop_with_led_off();
    }

    if (clock_init_48mhz() == 0U) {
        stop_with_led_off();
    }

    SYST_RVR = 48000U - 1U;      // 48 MHz下每48000个周期产生1 ms中断
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;

    if (uart4_init() == 0U) {
        stop_with_led_off();
    }
    if (bxcan_enter_and_exit_init() == 0U) {
        stop_with_led_off();
    }

    uint32_t last_toggle_ms = systick_millis;
    while (1) {
        if ((uint32_t)(systick_millis - last_toggle_ms) >= 1000U) {
            last_toggle_ms += 1000U;
            GPIOB_ODR ^= (1U << 1); // 正常状态每1 s翻转RUN灯
        }
    }
}

void Default_Handler(void)
{
    stop_with_led_off();
}
