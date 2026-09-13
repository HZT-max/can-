# 07：只加入 bxCAN 底层初始化

本版从实物已验证的 `06_uart4_init` 继续，只增加：

```text
CAN RX：PB8 AF4
CAN TX：PB9 AF4
CAN外设时钟和复位
PCLK1：48 MHz
CAN速率：1 Mbit/s
canardSTM32ComputeCANTimings()
canardSTM32Init()
```

本版复用完整候选版中同一份开源 `canard_stm32` bxCAN底层驱动，但不加入libcanard协议栈、DroneCAN消息、NodeStatus或串口数据收发。

实物结果：RUN不亮。因为06版已正常闪烁，07新增的bxCAN底层初始化是当前首个明确失败层。

指示灯规则：

- 正常：CAN底层成功退出初始化模式，RUN每1秒翻转一次。
- 异常：CAN时序计算或底层初始化失败，RUN保持不亮。

`canardSTM32Init()` 退出初始化模式前需要在CAN RX上看到11个连续隐性位。测试时应保证CAN收发器供电、CANH/CANL接线正确，且总线有正确终端电阻。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
