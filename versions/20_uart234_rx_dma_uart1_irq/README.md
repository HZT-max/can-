# 20：UART2/3/4 DMA接收、UART1中断接收完整版

本版本以已验证可运行的第17版为基准，保留完整DroneCAN双向串口隧道业务。
UART2、UART3、UART4改为循环DMA接收，UART1改为RXNE中断加静态环形缓冲，解决主循环轮询连续GPS数据时可能出现的丢字节问题。

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
UART1接收方式       USART1 RXNE中断，256字节静态环形缓冲
UART2接收方式       DMA1通道5，256字节循环缓冲
UART3接收方式       DMA1通道3，256字节循环缓冲（SYSCFG重映射）
UART4接收方式       DMA1通道6，256字节循环缓冲
四路发送方式        主循环轮询TXE，256字节静态发送缓冲/路
```

飞控发送首个 `uavcan.tunnel.Targetted` 消息后，固件记录该飞控的节点ID、协议和要求的波特率，并建立对应串口的回传路由。飞控持续发送的空缓冲 keepalive 会保持路由；3秒未收到请求则停止该路串口向CAN回传。

节点每秒发布一次 `uavcan.protocol.NodeStatus`，并响应 `uavcan.protocol.GetNodeInfo` 请求；地面站查询时显示节点名称 `com.airbrain.canuart4`。

串口字节不会被解析或修改。UART收到的数据以最多120字节为一组，封装成 `uavcan.tunnel.Targetted`；libcanard再按DroneCAN传输层规则自动拆成若干个经典CAN帧。反方向收到完整DroneCAN传输后，libcanard先完成多帧重组和CRC检查，固件再根据 `serial_id` 写入UART1至UART4。

## 指示灯含义

- RUN（PB1/LED3）：完整业务进入主循环后每1秒翻转一次。
- LED2（PB0/BL）：CAN硬件报告TXOK时点亮100 ms；NodeStatus每秒发送，正常连接时应每秒短闪。
- RUN正常而LED2不闪：软件在运行，但CAN发送没有获得总线上其他节点的ACK。

CAN初始化只尝试一次。初始化失败时程序停在异常状态，RUN和LED2保持熄灭；初始化成功后RUN每1秒翻转。PB8不启用内部上拉，直接读取飞线修正后U6 RXD输出的真实总线状态。

DMA只负责把连续收到的UART2/3/4字节搬入循环缓冲；主循环仍按serial_id分别封装并发送，不会混淆三个串口。DMA循环接收通过读取CNDTR位置消费数据，不启用DMA中断。UART1中断处理函数只做逐字节入环形缓冲，DroneCAN编码和发送仍在主循环执行。

## 内存原则

本版本无RTOS、无堆分配、无UART线程。libcanard使用固定6144字节静态内存池，四路UART各使用静态接收和发送缓冲，三路DMA接收缓冲和UART1中断接收缓冲也均为静态内存。链接脚本固定保留2 KiB运行栈，并在链接时检查静态区不得与栈相撞。

## 编译和烧录

在Linux或虚拟机中执行：

```bash
./build.sh
```

J-Link烧录文件：`firmware.bin`

烧录地址：`0x08000000`

不需要连接串口外设即可测试。只连接飞控CAN，NodeStatus就会触发LED2活动。

`tests/test_host.sh` 是不访问硬件的协议回环测试：它用节点10构造一条含120字节串口数据的Targetted消息。经典DroneCAN启用TAO后，有效载荷实际为126字节；再加上传输级CRC，libcanard将其拆成19个经典CAN帧，并在节点125端重组、校验和解码回完全一致的字段与数据。
