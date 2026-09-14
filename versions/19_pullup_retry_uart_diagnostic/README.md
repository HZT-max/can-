# 19：上拉循环初始化串口诊断版

本版本保留第16版全部业务功能、CAN_RX弱上拉、CAN初始化持续重试和双LED状态。
增加UART1 115200 ASCII诊断输出，不需要测量MCU或收发器细小引脚。

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

## 指示灯含义

- RUN（PB1/LED3）：完整业务进入主循环后每1秒翻转一次。
- LED2（PB0/BL）：CAN硬件报告TXOK时点亮100 ms；NodeStatus每秒发送，正常连接时应每秒短闪。
- RUN正常而LED2不闪：软件在运行，但CAN发送没有获得总线上其他节点的ACK。

CAN初始化失败不会锁死在单次结果上。程序保持RUN灯熄灭并持续重试；当收发器和总线就绪后会自动进入正常运行。

PB8（CAN_RX）启用了弱上拉，作用是收发器尚未上电或输出高阻时保持隐性电平；收发器正常工作后，其推挽RXD输出会覆盖该弱上拉。

## UART1诊断

USB转TTL接线：拓展器TX1接USB转TTL的RX，双方GND相连。串口设置为115200、8数据位、无校验、1停止位。

启动时输出CAN初始化结果，随后每秒输出ESR、TEC、REC、LEC、提交发送数、成功ACK数、发送错误数、仲裁丢失数和接收帧数。`LEC=ACK`表示发送帧没有得到其他节点应答；`BUS_OFF`表示发送错误累计后控制器退出总线。

## 内存原则

本版本无RTOS、无堆分配、无UART线程。libcanard使用固定6144字节静态内存池，四路UART各使用静态接收和发送缓冲。链接脚本固定保留2 KiB运行栈，并在链接时检查静态区不得与栈相撞。

## 编译和烧录

在Linux或虚拟机中执行：

```bash
./build.sh
```

J-Link烧录文件：`firmware.bin`

烧录地址：`0x08000000`

不需要连接串口外设即可测试。只连接飞控CAN，NodeStatus就会触发LED2活动。

`tests/test_host.sh` 是不访问硬件的协议回环测试：它用节点10构造一条含120字节串口数据的Targetted消息。经典DroneCAN启用TAO后，有效载荷实际为126字节；再加上传输级CRC，libcanard将其拆成19个经典CAN帧，并在节点125端重组、校验和解码回完全一致的字段与数据。
