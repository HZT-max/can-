# 09：只检查 bxCAN 进入初始化模式

本版从实物已验证的 `06_uart4_init` 出发，在保留48 MHz时钟、SysTick和四路UART的基础上，只执行：

```text
PB8 = CAN RX，AF4
PB9 = CAN TX，AF4
开启并复位CAN外设时钟
清除MCR.SLEEP
设置MCR.INRQ = 1
等待MSR.INAK = 1
```

本版不写CAN波特率，不退出初始化模式，不调用 `canardSTM32Init()`，不收发CAN数据。因此该步不需要CANH/CANL上存在其他节点或通信数据。

实物结果：拓展器单独供电和由飞控CAN口供电时都正常闪烁。这证明CAN外设时钟、复位、寄存器地址和进入初始化模式均正常。

指示灯规则：

- 正常：在100 ms内看到 `MSR.INAK=1`，RUN每1秒翻转一次。
- 异常：超时未进入初始化模式，RUN保持不亮。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
