#!/usr/bin/env bash
set -euo pipefail

arm-none-eabi-gcc \
    -mcpu=cortex-m0 \
    -mthumb \
    -Os \
    -ffreestanding \
    -fdata-sections \
    -ffunction-sections \
    -fno-builtin \
    -D_XOPEN_SOURCE=600 \
    -DNDEBUG \
    -Ilibcanard \
    -Ilibcanard/drivers/stm32 \
    -nostdlib \
    -Wl,--gc-sections \
    -Wl,-T,linker.ld \
    main.c memory.c libcanard/drivers/stm32/canard_stm32.c \
    -lgcc \
    -o firmware.elf

arm-none-eabi-objcopy -O binary firmware.elf firmware.bin
sha256sum firmware.bin
