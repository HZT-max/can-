# 06：只加入四路 UART 初始化

本版从实物已验证的 `05_systick_timebase` 继续，只增加四路 UART 引脚和 115200 波特率初始化：

```text
serial_id 1：USART1，PA9 TX / PA10 RX，AF1
serial_id 2：USART2，PA2 TX / PA3 RX，AF1
serial_id 3：USART3，PB10 TX / PB11 RX，AF4
serial_id 4：USART4，PA0 TX / PA1 RX，AF4
初始波特率：115200
```

本阶段不收发UART数据，不包含CAN和libcanard。

实物结果：正常闪烁，四路UART引脚、时钟和寄存器初始化已验证不会导致启动故障。

指示灯统一规则：

- 正常：RUN每1秒翻转一次。
- 异常：RUN保持不亮。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
