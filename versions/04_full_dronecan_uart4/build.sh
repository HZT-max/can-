#!/usr/bin/env bash
set -euo pipefail

TOOLCHAIN_DIR="${TOOLCHAIN_DIR:-$HOME/.local/toolchains/gcc-arm-none-eabi-10-2020-q4-major/bin}"
CC="${TOOLCHAIN_DIR}/arm-none-eabi-gcc"
OBJCOPY="${TOOLCHAIN_DIR}/arm-none-eabi-objcopy"
OBJDUMP="${TOOLCHAIN_DIR}/arm-none-eabi-objdump"
SIZE="${TOOLCHAIN_DIR}/arm-none-eabi-size"

COMMON_FLAGS=(
    -mcpu=cortex-m0
    -mthumb
    -std=c11
    -Os
    -ffreestanding
    -fno-builtin
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Werror
    -Wstack-usage=1024
    -D_XOPEN_SOURCE=600
    -DCANARD_ENABLE_TAO_OPTION=1
    -DNDEBUG
    -Ilibcanard
    -Ilibcanard/drivers/stm32
    -Idsdl
)

SOURCES=(
    main.c
    memory.c
    libcanard/canard.c
    libcanard/drivers/stm32/canard_stm32.c
    dsdl/uavcan.tunnel.Protocol.c
    dsdl/uavcan.tunnel.Targetted.c
    dsdl/uavcan.protocol.NodeStatus.c
    dsdl/uavcan.protocol.GetNodeInfo_req.c
    dsdl/uavcan.protocol.GetNodeInfo_res.c
    dsdl/uavcan.protocol.HardwareVersion.c
    dsdl/uavcan.protocol.SoftwareVersion.c
)

"${CC}" "${COMMON_FLAGS[@]}" \
    -nostdlib \
    -Wl,--gc-sections \
    -Wl,--build-id=none \
    -Wl,-Map,firmware.map \
    -Wl,-T,linker.ld \
    "${SOURCES[@]}" \
    -lgcc \
    -o firmware.elf

"${OBJCOPY}" -O binary firmware.elf firmware.bin
"${OBJDUMP}" -h firmware.elf > firmware.sections.txt
"${OBJDUMP}" -d firmware.elf > firmware.disassembly.txt
"${SIZE}" -A firmware.elf > firmware.size.txt
sha256sum firmware.bin > firmware.sha256

cat firmware.size.txt
cat firmware.sha256
