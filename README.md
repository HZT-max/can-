# STM32F072 CAN/UART 拓展器分阶段验证

本仓库以已在实物上验证能够驱动 PB1 RUN 灯闪烁的裸机程序为唯一基准，逐层加入启动、时钟、操作系统、CAN 和串口功能。每个阶段都保留完整源码、链接脚本、可烧录 BIN 和 ELF，并用独立 Git 提交与标签管理。

## 固定硬件

- MCU：STM32F072C8T6
- RUN：PB1，高电平点亮
- CAN：PB8 RX / PB9 TX
- UART1：PA9 TX / PA10 RX
- 烧录地址：`0x08000000`

## 版本状态

| 目录 | 变更 | 实物结果 |
| --- | --- | --- |
| `00_baremetal_led_baseline` | 原始裸机 PB1 闪灯 | 已验证：能闪 |
| `01_c_runtime_init` | 增加 `.data` 复制和 `.bss` 清零，但装载地址未对齐 | 已验证：不亮，未对齐访问导致 HardFault |
| `02_c_runtime_aligned` | 仅把 `.data` 的 Flash 装载地址对齐到4字节 | 已验证：能闪 |
| `03_hse_pll_48mhz` | 加入8 MHz HSE和PLL，切换系统时钟到48 MHz | 已验证：能闪 |
| `04_full_dronecan_uart4` | 四路UART、DroneCAN Targetted双向隧道、NodeStatus、GetNodeInfo | 实物失败：RUN快速闪烁，程序进入了主动故障分支 |
| `05_systick_timebase` | 从03重新出发，只增加SysTick 1 ms时基 | 已验证：正常闪烁 |
| `06_uart4_init` | 在05基础上只增加四路UART引脚和115200初始化 | 已验证：正常闪烁 |
| `07_bxcan_init` | 在06基础上只增加PB8/PB9和1 Mbit/s bxCAN底层初始化 | 已验证失败：RUN不亮，bxCAN底层初始化返回失败 |
| `08_full_bypass_can_hardware` | 保留完整应用路径，但不初始化或访问bxCAN硬件 | 已验证：正常闪烁 |
| `09_bxcan_enter_init` | 从06出发，只检查bxCAN能否进入初始化模式 | 已验证：有无CAN收发器供电均正常闪烁 |
| `10_bxcan_exit_init` | 在09基础上写入1 Mbit/s时序并检查bxCAN能否退出初始化模式 | 已验证失败：单独供电和接入飞控均不能稳定退出 |
| `11_bxcan_internal_loopback` | 使用静默内部回环隔离收发器和外部总线，再检查bxCAN退出初始化 | 待实物验证 |
| `12_full_dronecan_uart4_retry` | 四路UART、完整DroneCAN隧道和节点服务；增加CAN上电重试与RX上拉 | 启动正常，但节点125尚未被地面站发现 |
| `13_full_retry_no_canrx_pullup` | 保留完整功能与CAN重试，仅取消CAN_RX弱上拉以定位根因 | 已验证失败：仅前两次启动成功，随后不稳定 |
| `14_full_canrx_pullup_single_init` | 保留完整功能与CAN_RX弱上拉，CAN只初始化一次以定位根因 | 启动正常，但节点125尚未被地面站发现 |
| `15_full_can_tx_diagnostic` | 保留第12版完整功能，RUN灯改为显示bxCAN发送确认或错误 | 待实物验证 |
| `16_full_dual_led_can_activity` | RUN保持原业务状态，LED2显示获得ACK的CAN发送活动 | 待实物验证 |
| `17_full_single_init_no_pullup` | 飞线修正后取消CAN_RX上拉，CAN仅初始化一次；保留完整业务和双LED诊断 | 待实物验证 |
| `18_retry_no_pullup` | 飞线修正后保持无CAN_RX上拉，CAN初始化失败时每100 ms循环重试 | 待实物验证 |

`03_hse_pll_48mhz` 已经实物确认闪灯，后续完整候选固件必须继承该版的启动、时钟和链接脚本。

## Git 约定

- 每个阶段使用独立目录保存完整源码与固件。
- 每个阶段对应一个独立提交和 `stage-XX` 标签。
- `origin` 指向 `https://github.com/HZT-max/can-.git`。
