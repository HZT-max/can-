#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <canard.h>
#include <canard_stm32.h>
#include <uavcan.protocol.GetNodeInfo.h>
#include <uavcan.protocol.NodeStatus.h>
#include <uavcan.tunnel.Targetted.h>

#define SYSTEM_CLOCK_HZ             48000000U
#define CAN_BITRATE                 1000000U
#define LOCAL_NODE_ID               125U
#define UART_PORT_COUNT             4U
#define UART_DEFAULT_BAUDRATE       115200U
#define UART_TX_BUFFER_SIZE         256U
#define UART_RX_PACKET_SIZE         120U
#define UART_ROUTE_TIMEOUT_MS       3000U
#define UART_PACKET_IDLE_MS         2U
#define CANARD_MEMORY_POOL_SIZE     6144U
#define STM32_UNIQUE_ID_ADDRESS     0x1FFFF7ACU

#define RCC_BASE                    0x40021000U
#define RCC_CR                      (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_CFGR                    (*(volatile uint32_t *)(RCC_BASE + 0x04U))
#define RCC_APB2RSTR                (*(volatile uint32_t *)(RCC_BASE + 0x0CU))
#define RCC_APB1RSTR                (*(volatile uint32_t *)(RCC_BASE + 0x10U))
#define RCC_AHBENR                  (*(volatile uint32_t *)(RCC_BASE + 0x14U))
#define RCC_APB2ENR                 (*(volatile uint32_t *)(RCC_BASE + 0x18U))
#define RCC_APB1ENR                 (*(volatile uint32_t *)(RCC_BASE + 0x1CU))
#define RCC_CFGR2                   (*(volatile uint32_t *)(RCC_BASE + 0x2CU))
#define FLASH_ACR                   (*(volatile uint32_t *)0x40022000U)

#define RCC_CR_HSEON                (1U << 16)
#define RCC_CR_HSERDY               (1U << 17)
#define RCC_CR_PLLON                (1U << 24)
#define RCC_CR_PLLRDY               (1U << 25)
#define RCC_CFGR_PLLSRC_HSE         (2U << 15)
#define RCC_CFGR_PLLMUL6            (4U << 18)
#define RCC_CFGR_SW_PLL             2U
#define RCC_CFGR_SWS_PLL            (2U << 2)
#define FLASH_ACR_48MHZ             0x11U

#define RCC_AHBENR_GPIOAEN          (1U << 17)
#define RCC_AHBENR_GPIOBEN          (1U << 18)
#define RCC_APB2ENR_USART1EN        (1U << 14)
#define RCC_APB1ENR_USART2EN        (1U << 17)
#define RCC_APB1ENR_USART3EN        (1U << 18)
#define RCC_APB1ENR_UART4EN         (1U << 19)
#define RCC_APB1ENR_CANEN           (1U << 25)

#define CAN_TSR                     (*(volatile uint32_t *)0x40006408U)
#define CAN_ESR                     (*(volatile uint32_t *)0x40006418U)
#define CAN_TSR_TXOK_MASK           ((1U << 1U) | (1U << 9U) | (1U << 17U))
#define CAN_TSR_RQCP_MASK           ((1U << 0U) | (1U << 8U) | (1U << 16U))
#define CAN_TSR_ALST_MASK           ((1U << 2U) | (1U << 10U) | (1U << 18U))
#define CAN_TSR_TERR_MASK           ((1U << 3U) | (1U << 11U) | (1U << 19U))
#define CAN_ESR_EWGF                (1U << 0U)
#define CAN_ESR_EPVF                (1U << 1U)
#define CAN_ESR_BOFF                (1U << 2U)
#define CAN_ESR_LEC_SHIFT           4U
#define CAN_ESR_LEC_MASK            (7U << CAN_ESR_LEC_SHIFT)
#define CAN_ESR_TEC_SHIFT           16U
#define CAN_ESR_REC_SHIFT           24U

#define SYST_CSR                    (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR                    (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR                    (*(volatile uint32_t *)0xE000E018U)
#define SYST_CSR_ENABLE             (1U << 0)
#define SYST_CSR_TICKINT            (1U << 1)
#define SYST_CSR_CLKSOURCE          (1U << 2)

#define USART_CR1_UE                (1U << 0)
#define USART_CR1_RE                (1U << 2)
#define USART_CR1_TE                (1U << 3)
#define USART_ISR_PE                (1U << 0)
#define USART_ISR_FE                (1U << 1)
#define USART_ISR_NE                (1U << 2)
#define USART_ISR_ORE               (1U << 3)
#define USART_ISR_RXNE              (1U << 5)
#define USART_ISR_TXE               (1U << 7)
#define USART_ERROR_MASK            (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE)

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
    volatile uint32_t BRR;
} GPIORegisters;

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

#define GPIOA                      ((GPIORegisters *)0x48000000U)
#define GPIOB                      ((GPIORegisters *)0x48000400U)
#define USART1_REGS                ((USARTRegisters *)0x40013800U)
#define USART2_REGS                ((USARTRegisters *)0x40004400U)
#define USART3_REGS                ((USARTRegisters *)0x40004800U)
#define UART4_REGS                 ((USARTRegisters *)0x40004C00U)

typedef struct {
    USARTRegisters *regs;
    uint32_t baudrate;
    uint32_t last_request_ms;
    uint32_t last_rx_ms;
    uint8_t remote_node_id;
    uint8_t protocol;
    uint8_t serial_id;
    bool route_valid;
    uint8_t rx_packet[UART_RX_PACKET_SIZE];
    uint8_t rx_packet_len;
    uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
    uint16_t tx_head;
    uint16_t tx_tail;
    uint32_t dropped_rx_bytes;
    uint32_t dropped_tx_bytes;
} UARTPort;

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);

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

volatile uint32_t initialized_marker = 0x13579BDFU;
volatile uint32_t zero_marker;
static volatile uint32_t g_millis;
static CanardInstance g_canard;
static uint8_t g_canard_memory_pool[CANARD_MEMORY_POOL_SIZE] __attribute__((aligned(8)));
static uint8_t g_tunnel_transfer_id;
static uint8_t g_node_status_transfer_id;
static uint32_t g_led2_off_ms;
static uint32_t g_can_tx_submitted;
static uint32_t g_can_tx_ok;
static uint32_t g_can_tx_error;
static uint32_t g_can_arbitration_lost;
static uint32_t g_can_rx_frames;
static uint8_t g_last_can_lec;
static UARTPort g_uart_ports[UART_PORT_COUNT] = {
    { .regs = USART1_REGS, .baudrate = UART_DEFAULT_BAUDRATE, .serial_id = 1U },
    { .regs = USART2_REGS, .baudrate = UART_DEFAULT_BAUDRATE, .serial_id = 2U },
    { .regs = USART3_REGS, .baudrate = UART_DEFAULT_BAUDRATE, .serial_id = 3U },
    { .regs = UART4_REGS,  .baudrate = UART_DEFAULT_BAUDRATE, .serial_id = 4U },
};

static uint32_t millis(void)
{
    return g_millis;
}

static uint64_t micros64(void)
{
    return (uint64_t)millis() * 1000ULL;
}

void SysTick_Handler(void)
{
    g_millis++;
}

/* libcanard bxCAN驱动在控制器进入初始化模式时调用此函数。 */
int usleep(unsigned int usec)
{
    const uint32_t wait_ms = (usec + 999U) / 1000U;
    const uint32_t start_ms = millis();
    while ((uint32_t)(millis() - start_ms) < wait_ms) {
    }
    return 0;
}

/* 启动或运行异常时保持RUN灯熄灭。 */
static void stop_with_led_off(void)
{
    GPIOB->ODR &= ~((1U << 0) | (1U << 1));
    while (true) {
    }
}

/* 使用8 MHz HSE和PLL建立所有外设共用的48 MHz时钟。 */
static bool clock_init_48mhz(void)
{
    uint32_t timeout = 1000000U;

    RCC_CR |= RCC_CR_HSEON;
    while (((RCC_CR & RCC_CR_HSERDY) == 0U) && (--timeout != 0U)) {
    }
    if (timeout == 0U) {
        return false;
    }

    RCC_CFGR2 &= ~0xFU;
    RCC_CFGR &= ~((3U << 15) | (0xFU << 18));
    RCC_CFGR |= RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMUL6;
    RCC_CR |= RCC_CR_PLLON;

    timeout = 1000000U;
    while (((RCC_CR & RCC_CR_PLLRDY) == 0U) && (--timeout != 0U)) {
    }
    if (timeout == 0U) {
        return false;
    }

    FLASH_ACR = FLASH_ACR_48MHZ;
    RCC_CFGR = (RCC_CFGR & ~3U) | RCC_CFGR_SW_PLL;

    timeout = 1000000U;
    while (((RCC_CFGR & (3U << 2)) != RCC_CFGR_SWS_PLL) && (--timeout != 0U)) {
    }
    return timeout != 0U;
}

static void gpio_config_output(GPIORegisters *gpio, uint8_t pin)
{
    gpio->MODER = (gpio->MODER & ~(3U << (pin * 2U))) | (1U << (pin * 2U));
    gpio->OTYPER &= ~(1U << pin);
    gpio->OSPEEDR |= (3U << (pin * 2U));
    gpio->PUPDR &= ~(3U << (pin * 2U));
}

static void gpio_config_af(GPIORegisters *gpio, uint8_t pin, uint8_t af)
{
    const uint8_t afr_index = pin / 8U;
    const uint8_t afr_shift = (pin % 8U) * 4U;
    gpio->MODER = (gpio->MODER & ~(3U << (pin * 2U))) | (2U << (pin * 2U));
    gpio->OTYPER &= ~(1U << pin);
    gpio->OSPEEDR |= (3U << (pin * 2U));
    gpio->PUPDR &= ~(3U << (pin * 2U));
    gpio->AFR[afr_index] = (gpio->AFR[afr_index] & ~(0xFU << afr_shift)) |
                           ((uint32_t)af << afr_shift);
}

/* 配置PB0 LED2、PB1 RUN、四路UART和PB8/PB9 CAN复用引脚。 */
static void gpio_init(void)
{
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

    gpio_config_output(GPIOB, 0U);
    gpio_config_output(GPIOB, 1U);
    GPIOB->ODR &= ~((1U << 0) | (1U << 1));

    gpio_config_af(GPIOA, 9U, 1U);
    gpio_config_af(GPIOA, 10U, 1U);
    gpio_config_af(GPIOA, 2U, 1U);
    gpio_config_af(GPIOA, 3U, 1U);
    gpio_config_af(GPIOB, 10U, 4U);
    gpio_config_af(GPIOB, 11U, 4U);
    gpio_config_af(GPIOA, 0U, 4U);
    gpio_config_af(GPIOA, 1U, 4U);

    gpio_config_af(GPIOB, 8U, 4U);
    gpio_config_af(GPIOB, 9U, 4U);
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << 16U)) | (1U << 16U); // 收发器上电前保持CAN_RX隐性
}

static void uart_apply_baudrate(UARTPort *port, uint32_t baudrate)
{
    if ((baudrate < 1200U) || (baudrate > 3000000U)) {
        return;
    }

    const uint32_t divider = (SYSTEM_CLOCK_HZ + (baudrate / 2U)) / baudrate;
    port->regs->CR1 = 0U;
    port->regs->CR2 = 0U;
    port->regs->CR3 = 0U;
    port->regs->BRR = divider;
    port->regs->ICR = 0xFFFFFFFFU;
    port->regs->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;
    port->baudrate = baudrate;
}

/* 四路UART均静态初始化，不创建线程和动态缓冲区。 */
static void uart_init_all(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN | RCC_APB1ENR_UART4EN;

    RCC_APB2RSTR |= RCC_APB2ENR_USART1EN;
    RCC_APB2RSTR &= ~RCC_APB2ENR_USART1EN;
    RCC_APB1RSTR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN | RCC_APB1ENR_UART4EN;
    RCC_APB1RSTR &= ~(RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN | RCC_APB1ENR_UART4EN);

    for (uint8_t i = 0U; i < UART_PORT_COUNT; i++) {
        uart_apply_baudrate(&g_uart_ports[i], UART_DEFAULT_BAUDRATE);
    }
}

/* 诊断版通过UART1 TX输出ASCII文本，USB转TTL只需接收，不占用UART1 RX。 */
static void debug_uart1_putc(char value)
{
    uint32_t timeout = SYSTEM_CLOCK_HZ / 10U;
    while (((USART1_REGS->ISR & USART_ISR_TXE) == 0U) && (--timeout != 0U)) {
    }
    if (timeout != 0U) {
        USART1_REGS->TDR = (uint8_t)value;
    }
}

static void debug_uart1_puts(const char *text)
{
    while (*text != '\0') {
        debug_uart1_putc(*text++);
    }
}

static void debug_uart1_u32(uint32_t value)
{
    char digits[10];
    uint8_t length = 0U;
    do {
        digits[length++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) && (length < sizeof(digits)));
    while (length > 0U) {
        debug_uart1_putc(digits[--length]);
    }
}

static void debug_uart1_hex32(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    for (int8_t shift = 28; shift >= 0; shift -= 4) {
        debug_uart1_putc(hex[(value >> (uint8_t)shift) & 0xFU]);
    }
}

static const char *can_lec_name(uint8_t lec)
{
    static const char *const names[] = {
        "NONE", "STUFF", "FORM", "ACK", "BIT_RECESSIVE", "BIT_DOMINANT", "CRC", "SOFTWARE"
    };
    return names[lec & 7U];
}

/* 每秒输出CAN控制器状态；用于不用示波器时区分无ACK、位错误和Bus-Off。 */
static void debug_can_report(void)
{
    const uint32_t esr = CAN_ESR;
    const uint8_t current_lec = (uint8_t)((esr & CAN_ESR_LEC_MASK) >> CAN_ESR_LEC_SHIFT);
    const uint8_t report_lec = (g_last_can_lec != 0U) ? g_last_can_lec : current_lec;

    debug_uart1_puts("CAN ESR=0x");
    debug_uart1_hex32(esr);
    debug_uart1_puts(" TEC=");
    debug_uart1_u32((esr >> CAN_ESR_TEC_SHIFT) & 0xFFU);
    debug_uart1_puts(" REC=");
    debug_uart1_u32((esr >> CAN_ESR_REC_SHIFT) & 0xFFU);
    debug_uart1_puts(" LEC=");
    debug_uart1_puts(can_lec_name(report_lec));
    debug_uart1_puts(" TX_SUB=");
    debug_uart1_u32(g_can_tx_submitted);
    debug_uart1_puts(" TX_OK=");
    debug_uart1_u32(g_can_tx_ok);
    debug_uart1_puts(" TX_ERR=");
    debug_uart1_u32(g_can_tx_error);
    debug_uart1_puts(" ALST=");
    debug_uart1_u32(g_can_arbitration_lost);
    debug_uart1_puts(" RX=");
    debug_uart1_u32(g_can_rx_frames);
    if ((esr & CAN_ESR_BOFF) != 0U) {
        debug_uart1_puts(" BUS_OFF");
    } else if ((esr & CAN_ESR_EPVF) != 0U) {
        debug_uart1_puts(" ERROR_PASSIVE");
    } else if ((esr & CAN_ESR_EWGF) != 0U) {
        debug_uart1_puts(" ERROR_WARNING");
    }
    debug_uart1_puts("\r\n");
    g_last_can_lec = 0U;
}

static bool uart_tx_enqueue(UARTPort *port, const uint8_t *data, uint8_t length)
{
    bool complete = true;
    for (uint8_t i = 0U; i < length; i++) {
        const uint16_t next = (uint16_t)((port->tx_head + 1U) % UART_TX_BUFFER_SIZE);
        if (next == port->tx_tail) {
            port->dropped_tx_bytes += (uint32_t)(length - i);
            complete = false;
            break;
        }
        port->tx_buffer[port->tx_head] = data[i];
        port->tx_head = next;
    }
    return complete;
}

static void uart_poll_tx(UARTPort *port)
{
    if ((port->tx_head != port->tx_tail) && ((port->regs->ISR & USART_ISR_TXE) != 0U)) {
        port->regs->TDR = port->tx_buffer[port->tx_tail];
        port->tx_tail = (uint16_t)((port->tx_tail + 1U) % UART_TX_BUFFER_SIZE);
    }
}

/* 回应节点信息请求，使地面站能显示节点名称、版本和芯片唯一编号。 */
static void respond_get_node_info(CanardInstance *ins, CanardRxTransfer *transfer)
{
    static struct uavcan_protocol_GetNodeInfoResponse response;
    static uint8_t payload[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
    static const uint8_t node_name[] = "com.airbrain.canuart4";
    const uint8_t *unique_id = (const uint8_t *)STM32_UNIQUE_ID_ADDRESS;

    memset(&response, 0, sizeof(response));
    response.status.uptime_sec = millis() / 1000U;
    response.status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    response.status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    response.software_version.major = 1U;
    response.software_version.minor = 0U;
    response.hardware_version.major = 1U;
    response.hardware_version.minor = 0U;

    for (uint8_t i = 0U; i < 12U; i++) {
        response.hardware_version.unique_id[i] = unique_id[i];
    }
    for (uint8_t i = 12U; i < 16U; i++) {
        response.hardware_version.unique_id[i] = (uint8_t)(unique_id[i - 12U] ^ unique_id[i - 8U]);
    }

    response.name.len = (uint8_t)(sizeof(node_name) - 1U);
    memcpy(response.name.data, node_name, response.name.len);

    const uint16_t payload_length =
        (uint16_t)uavcan_protocol_GetNodeInfoResponse_encode(&response, payload, true);
    (void)canardRequestOrRespond(ins,
                                 transfer->source_node_id,
                                 UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                                 UAVCAN_PROTOCOL_GETNODEINFO_ID,
                                 &transfer->transfer_id,
                                 transfer->priority,
                                 CanardResponse,
                                 payload,
                                 payload_length);
}

/* 只接收发往本节点的Targetted消息，并按serial_id写到对应物理UART。 */
static void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    if ((transfer->transfer_type == CanardTransferTypeRequest) &&
        (transfer->data_type_id == UAVCAN_PROTOCOL_GETNODEINFO_ID)) {
        respond_get_node_info(ins, transfer);
        return;
    }
    if ((transfer->transfer_type != CanardTransferTypeBroadcast) ||
        (transfer->data_type_id != UAVCAN_TUNNEL_TARGETTED_ID)) {
        return;
    }

    struct uavcan_tunnel_Targetted message;
    if (uavcan_tunnel_Targetted_decode(transfer, &message)) {
        return;
    }
    if (message.target_node != LOCAL_NODE_ID) {
        return;
    }

    int8_t serial_id = message.serial_id;
    if (serial_id == -1) {
        serial_id = 1;
    }
    if ((serial_id < 1) || (serial_id > (int8_t)UART_PORT_COUNT)) {
        return;
    }

    UARTPort *port = &g_uart_ports[(uint8_t)serial_id - 1U];
    port->remote_node_id = transfer->source_node_id;
    port->protocol = message.protocol.protocol;
    port->last_request_ms = millis();
    port->route_valid = transfer->source_node_id != 0U;

    if ((message.baudrate != 0U) && (message.baudrate != port->baudrate)) {
        uart_apply_baudrate(port, message.baudrate);
    }
    if (message.buffer.len > 0U) {
        (void)uart_tx_enqueue(port, message.buffer.data, message.buffer.len);
    }
}

static bool should_accept_transfer(const CanardInstance *ins,
                                   uint64_t *out_signature,
                                   uint16_t data_type_id,
                                   CanardTransferType transfer_type,
                                   uint8_t source_node_id)
{
    (void)ins;
    (void)source_node_id;
    if ((transfer_type == CanardTransferTypeRequest) &&
        (data_type_id == UAVCAN_PROTOCOL_GETNODEINFO_ID)) {
        *out_signature = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
        return true;
    }
    if ((transfer_type == CanardTransferTypeBroadcast) &&
        (data_type_id == UAVCAN_TUNNEL_TARGETTED_ID)) {
        *out_signature = UAVCAN_TUNNEL_TARGETTED_SIGNATURE;
        return true;
    }
    return false;
}

static bool publish_uart_packet(UARTPort *port)
{
    if (!port->route_valid || (port->rx_packet_len == 0U)) {
        return false;
    }

    struct uavcan_tunnel_Targetted message;
    memset(&message, 0, sizeof(message));
    message.protocol.protocol = port->protocol;
    message.target_node = port->remote_node_id;
    message.serial_id = (int8_t)port->serial_id;
    message.baudrate = port->baudrate;
    message.buffer.len = port->rx_packet_len;
    memcpy(message.buffer.data, port->rx_packet, port->rx_packet_len);

    uint8_t payload[UAVCAN_TUNNEL_TARGETTED_MAX_SIZE];
    const uint16_t payload_length = (uint16_t)uavcan_tunnel_Targetted_encode(&message, payload, true);
    const int16_t result = canardBroadcast(&g_canard,
                                           UAVCAN_TUNNEL_TARGETTED_SIGNATURE,
                                           UAVCAN_TUNNEL_TARGETTED_ID,
                                           &g_tunnel_transfer_id,
                                           CANARD_TRANSFER_PRIORITY_MEDIUM,
                                           payload,
                                           payload_length);
    if (result > 0) {
        port->rx_packet_len = 0U;
        return true;
    }
    return false;
}

/* 轮询物理UART；满120字节或空闲2 ms时封装成一条DroneCAN隧道消息。 */
static void uart_poll_rx(UARTPort *port)
{
    const uint32_t now_ms = millis();
    if (port->route_valid &&
        ((uint32_t)(now_ms - port->last_request_ms) >= UART_ROUTE_TIMEOUT_MS)) {
        port->route_valid = false;
        port->rx_packet_len = 0U;
    }

    uint8_t reads = 0U;
    while (((port->regs->ISR & USART_ISR_RXNE) != 0U) && (reads < 32U)) {
        const uint8_t byte = (uint8_t)port->regs->RDR;
        reads++;

        if (!port->route_valid) {
            continue;
        }
        if (port->rx_packet_len >= UART_RX_PACKET_SIZE) {
            (void)publish_uart_packet(port);
        }
        if (port->rx_packet_len < UART_RX_PACKET_SIZE) {
            port->rx_packet[port->rx_packet_len++] = byte;
            port->last_rx_ms = now_ms;
        } else {
            port->dropped_rx_bytes++;
        }
    }

    const uint32_t error_flags = port->regs->ISR & USART_ERROR_MASK;
    if (error_flags != 0U) {
        port->regs->ICR = error_flags;
    }

    if ((port->rx_packet_len >= UART_RX_PACKET_SIZE) ||
        ((port->rx_packet_len > 0U) &&
         ((uint32_t)(now_ms - port->last_rx_ms) >= UART_PACKET_IDLE_MS))) {
        (void)publish_uart_packet(port);
    }
}

static bool can_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_CANEN;
    RCC_APB1RSTR |= RCC_APB1ENR_CANEN;
    RCC_APB1RSTR &= ~RCC_APB1ENR_CANEN;

    CanardSTM32CANTimings timings;
    if (canardSTM32ComputeCANTimings(SYSTEM_CLOCK_HZ, CAN_BITRATE, &timings) < 0) {
        return false;
    }
    return canardSTM32Init(&timings, CanardSTM32IfaceModeNormal) == 0;
}

/* 收发器或总线晚于MCU上电时持续重试，成功前RUN灯保持熄灭。 */
static void can_init_with_retry(void)
{
    uint32_t attempts = 0U;
    while (!can_init()) {
        attempts++;
        debug_uart1_puts("CAN_INIT_FAIL attempt=");
        debug_uart1_u32(attempts);
        debug_uart1_puts(" PB8=");
        debug_uart1_u32((GPIOB->IDR >> 8U) & 1U);
        debug_uart1_puts("\r\n");
        GPIOB->ODR &= ~(1U << 1);
        const uint32_t retry_start_ms = millis();
        while ((uint32_t)(millis() - retry_start_ms) < 100U) {
        }
    }
    debug_uart1_puts("CAN_INIT_OK attempts=");
    debug_uart1_u32(attempts + 1U);
    debug_uart1_puts(" PB8=");
    debug_uart1_u32((GPIOB->IDR >> 8U) & 1U);
    debug_uart1_puts("\r\n");
}

static void can_process_rx(void)
{
    CanardCANFrame frame;
    for (uint8_t count = 0U; count < 32U; count++) {
        const int16_t result = canardSTM32Receive(&frame);
        if (result <= 0) {
            break;
        }
        g_can_rx_frames++;
        (void)canardHandleRxFrame(&g_canard, &frame, micros64());
    }
}

static void can_process_tx(void)
{
    while (true) {
        const CanardCANFrame *frame = canardPeekTxQueue(&g_canard);
        if (frame == NULL) {
            break;
        }

        const int16_t result = canardSTM32Transmit(frame);
        if (result > 0) {
            g_can_tx_submitted++;
            canardPopTxQueue(&g_canard);
        } else if (result < 0) {
            canardPopTxQueue(&g_canard);
        } else {
            break;
        }
    }
}

/* NodeStatus是节点在线心跳；RUN每秒翻转只表示完整业务主循环正在运行。 */
static void send_node_status(void)
{
    GPIOB->ODR ^= (1U << 1);

    struct uavcan_protocol_NodeStatus status;
    memset(&status, 0, sizeof(status));
    status.uptime_sec = millis() / 1000U;
    status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;

    uint8_t payload[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];
    const uint16_t payload_length = (uint16_t)uavcan_protocol_NodeStatus_encode(&status, payload, true);
    (void)canardBroadcast(&g_canard,
                          UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                          UAVCAN_PROTOCOL_NODESTATUS_ID,
                          &g_node_status_transfer_id,
                          CANARD_TRANSFER_PRIORITY_LOW,
                          payload,
                          payload_length);
}

/*
 * 主循环在每次CAN发送后调用；硬件置位TXOK代表帧已得到其他节点ACK。
 * 每次确认成功将PB0 LED2点亮100 ms，随后自动熄灭，作为CAN发送活动指示。
 */
static void update_can_activity_led(void)
{
    const uint32_t now_ms = millis();
    const uint32_t tsr = CAN_TSR;
    const uint32_t esr = CAN_ESR;
    const uint8_t lec = (uint8_t)((esr & CAN_ESR_LEC_MASK) >> CAN_ESR_LEC_SHIFT);

    if ((lec > 0U) && (lec < 7U)) {
        g_last_can_lec = lec;
    }

    if ((tsr & CAN_TSR_TXOK_MASK) != 0U) {
        g_can_tx_ok++;
        GPIOB->ODR |= (1U << 0);
        g_led2_off_ms = now_ms + 100U;
    }

    if ((tsr & CAN_TSR_TERR_MASK) != 0U) {
        g_can_tx_error++;
    }
    if ((tsr & CAN_TSR_ALST_MASK) != 0U) {
        g_can_arbitration_lost++;
    }

    if ((tsr & CAN_TSR_RQCP_MASK) != 0U) {
        CAN_TSR = tsr & CAN_TSR_RQCP_MASK; // 写1清除本次邮箱完成标志
    }

    if (((GPIOB->ODR & (1U << 0)) != 0U) &&
        ((int32_t)(now_ms - g_led2_off_ms) >= 0)) {
        GPIOB->ODR &= ~(1U << 0);
    }
}

static void app_main(void)
{
    gpio_init();

    if ((initialized_marker != 0x13579BDFU) || (zero_marker != 0U)) {
        stop_with_led_off();
    }
    if (!clock_init_48mhz()) {
        stop_with_led_off();
    }

    SYST_RVR = (SYSTEM_CLOCK_HZ / 1000U) - 1U;
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;

    uart_init_all();
    debug_uart1_puts("BOOT stage19 UART1=115200 CAN=1000000 NODE=125\r\n");
    canardInit(&g_canard,
               g_canard_memory_pool,
               sizeof(g_canard_memory_pool),
               on_transfer_received,
               should_accept_transfer,
               NULL);
    canardSetLocalNodeID(&g_canard, LOCAL_NODE_ID);

    can_init_with_retry();

    uint32_t last_status_ms = millis() - 1000U;
    while (true) {
        can_process_rx();
        can_process_tx();

        for (uint8_t i = 0U; i < UART_PORT_COUNT; i++) {
            uart_poll_tx(&g_uart_ports[i]);
            uart_poll_rx(&g_uart_ports[i]);
        }

        const uint32_t now_ms = millis();
        if ((uint32_t)(now_ms - last_status_ms) >= 1000U) {
            last_status_ms = now_ms;
            debug_can_report();
            canardCleanupStaleTransfers(&g_canard, micros64());
            send_node_status();
        }
        can_process_tx();
        update_can_activity_led();
    }
}

void Reset_Handler(void)
{
    uint32_t *source = &_sidata;
    for (uint32_t *destination = &_sdata; destination < &_edata; destination++) {
        *destination = *source++;
    }
    for (uint32_t *destination = &_sbss; destination < &_ebss; destination++) {
        *destination = 0U;
    }

    app_main();
    while (true) {
    }
}

void Default_Handler(void)
{
    stop_with_led_off();
}
