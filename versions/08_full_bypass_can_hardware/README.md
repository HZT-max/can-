# 08：完整应用路径，跳过 CAN 硬件层

这是针对07版CAN底层初始化失败的隔离诊断版。它保留：

```text
48 MHz时钟
SysTick 1 ms时基
4路UART及静态收发缓冲
libcanard固定内存池
uavcan.tunnel.Targetted编解码和路由代码
uavcan.protocol.NodeStatus
uavcan.protocol.GetNodeInfo
DroneCAN发送队列的创建和释放
```

本版故意不调用 `canardSTM32Init()`，也不访问bxCAN收发寄存器。应用层生成的CAN帧会直接从libcanard队列中丢弃，因此不会真正发到CANH/CANL。

指示灯规则：

- 正常：完整应用主循环持续运行，RUN每1秒翻转一次。
- 异常：RUN保持不亮。

如果本版正常闪烁，而07不亮，可以证明“完整版的立即启动失败”仅出现在bxCAN硬件初始化路径；不代表CAN通信已经可用。

J-Link烧录 `firmware.bin`，地址 `0x08000000`。
