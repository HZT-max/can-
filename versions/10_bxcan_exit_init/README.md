# 10：检查 bxCAN 写入时序并退出初始化模式

本版从实物已验证的 `09_bxcan_enter_init` 继续，新增：

```text
PCLK1 = 48 MHz
CAN BTR = 0x00050005
BRP = 6
BS1 = 6
BS2 = 1
标称速率 = 48 MHz / (6 x (1 + 6 + 1)) = 1 Mbit/s
清除MCR.INRQ
等待MSR.INAK = 0
```

本版不调用 `canardSTM32Init()`，不配置CAN滤波器，不收发CAN数据。

请分别在两种供电方式下观察：

1. 拓展器单独供电，CAN收发器无5V。
2. 飞控CAN口给拓展器VCC/GND供电，并连接CAN_H/CAN_L。

指示灯规则：

- 正常：在1秒内成功退出初始化模式，RUN每1秒翻转一次。
- 异常：`MSR.INAK` 一直为1，RUN保持不亮。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
