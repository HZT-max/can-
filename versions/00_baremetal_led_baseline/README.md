# 00：已验证裸机闪灯基准

这是实物复测能够闪灯的原始版本，没有操作系统、HAL、CAN、UART、堆或参数系统。

运行路径：

```text
复位向量 -> Reset_Handler -> 开启 GPIOB 时钟 -> PB1 输出 -> 循环翻转 PB1
```

烧录：

```text
文件：firmware.bin
地址：0x08000000
```

重新编译后 `firmware.bin` 应为 146 字节，SHA256 应为：

```text
4A06EA23F8EC399CC1F1C21D54A7F801703F2910C62BB62D830E89A8917D5258
```

