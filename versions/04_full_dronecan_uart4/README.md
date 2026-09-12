# 04：四路 UART 与 DroneCAN 完整候选版

本版本直接继承已通过实物闪灯验证的 `03_hse_pll_48mhz` 启动、时钟和链接方案，并加入完整的双向串口隧道。

## 固定配置

```text
MCU              STM32F072C8T6
系统时钟          48 MHz（8 MHz HSE，经PLL倍频6）
DroneCAN节点ID    125
CAN速率           1 Mbit/s
RUN               PB1，高电平点亮
CAN               PB8 RX / PB9 TX，AF4
串口1             PA9 TX / PA10 RX，AF1，serial_id=1
串口2             PA2 TX / PA3 RX，AF1，serial_id=2
串口3             PB10 TX / PB11 RX，AF4，serial_id=3
串口4             PA0 TX / PA1 RX，AF4，serial_id=4
串口初始波特率     115200
```

飞控发送首个 `uavcan.tunnel.Targetted` 消息后，固件记录该飞控的节点ID、协议和要求的波特率，并建立对应串口的回传路由。飞控持续发送的空缓冲 keepalive 会保持路由；3秒未收到请求则停止该路串口向CAN回传。

节点每秒发布一次 `uavcan.protocol.NodeStatus`，并响应 `uavcan.protocol.GetNodeInfo` 请求；地面站查询时显示节点名称 `com.airbrain.canuart4`。

串口字节不会被解析或修改。UART收到的数据以最多120字节为一组，封装成 `uavcan.tunnel.Targetted`；libcanard再按DroneCAN传输层规则自动拆成若干个经典CAN帧。反方向收到完整DroneCAN传输后，libcanard先完成多帧重组和CRC检查，固件再根据 `serial_id` 写入UART1至UART4。

## RUN灯含义

- 正常运行：每秒翻转一次，同时尝试发送一次 `uavcan.protocol.NodeStatus`。即使CAN未接通或拥塞，RUN灯仍继续翻转，用于单独证明主程序在运行。
- 快速连续闪烁：启动检查、48 MHz时钟或CAN初始化失败。
- 完全不亮：未执行到GPIO初始化，应优先检查烧录地址、向量表和供电。

## 内存原则

本版本无RTOS、无堆分配、无UART线程。libcanard使用固定6144字节静态内存池，四路UART各使用静态接收和发送缓冲。链接脚本固定保留2 KiB运行栈，并在链接时检查静态区不得与栈相撞。

## 编译和烧录

在Linux或虚拟机中执行：

```bash
./build.sh
```

J-Link烧录文件：`firmware.bin`

烧录地址：`0x08000000`

这是第一版完整候选固件。编译和静态检查通过不等于四路硬件通信已经实物通过；需要依次验证RUN、NodeStatus、CAN到各UART、各UART到CAN以及四路并发压力。

`tests/test_host.sh` 是不访问硬件的协议回环测试：它用节点10构造一条含120字节串口数据的Targetted消息。经典DroneCAN启用TAO后，有效载荷实际为126字节；再加上传输级CRC，libcanard将其拆成19个经典CAN帧，并在节点125端重组、校验和解码回完全一致的字段与数据。
