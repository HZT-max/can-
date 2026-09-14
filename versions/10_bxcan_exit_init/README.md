# 10：检查 bxCAN 写入时序并退出初始化模式（实物失败）

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

实物复测结果：

1. 拓展器单独供电：曾偶尔闪烁，随后多次复测均不闪。
2. 飞控CAN口供电并连接CAN_H/CAN_L：不闪。

结论：本阶段不能稳定退出bxCAN初始化模式。单独供电时收发器5V未建立，
CAN_RX可能不是稳定隐性电平；进入正常模式需要在CAN_RX上检测到连续隐性位，
所以此前偶发闪烁不能作为本阶段通过的依据。

指示灯规则：

- 正常：在1秒内成功退出初始化模式，RUN每1秒翻转一次。
- 异常：`MSR.INAK` 一直为1，RUN保持不亮。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
