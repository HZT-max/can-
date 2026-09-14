# 11：bxCAN静默内部回环诊断

本版从第10版继续，只修改CAN工作模式：

```text
保留1 Mbit/s时序
LBKM = 1：内部回环
SILM = 1：静默模式
```

该模式在MCU内部完成CAN收发路径闭环，不依赖外部CAN收发器、CANH、CANL或总线波特率。
本阶段仍不发送CAN帧，只验证bxCAN能否进入并退出初始化模式。

判断：

- RUN每1秒翻转：MCU内部bxCAN启动正常，第10版失败来自外部CAN_RX、收发器或总线条件。
- RUN始终不亮：MCU内部bxCAN仍不能退出初始化，需要继续检查寄存器配置或芯片状态。

烧录：

```text
文件：firmware.bin
J-Link地址：0x08000000
```
