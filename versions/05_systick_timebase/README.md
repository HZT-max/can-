# 05：只加入 SysTick 1 ms 时基

本版从实物已验证能闪的 `03_hse_pll_48mhz` 直接复制，不以完整版为基础。

相对03只增加：

```text
SysTick：48 MHz / 48000 = 1 kHz
SysTick_Handler：每1 ms将计数器加1
RUN：根据计数器每500 ms翻转一次
```

本版未加入 CAN、UART、libcanard、动态内存或RTOS。

实物结果：RUN稳定闪烁，SysTick异常向量、1 ms中断和48 MHz时基已验证正常。

判定依据：

- RUN稳定闪烁：SysTick异常向量、1 ms中断和48 MHz时基正常。
- RUN不闪：问题局限在SysTick增量，与CAN、UART和libcanard无关。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
