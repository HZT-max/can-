#!/usr/bin/env bash
set -euo pipefail

arm-none-eabi-gcc \
    -mcpu=cortex-m0 \
    -mthumb \
    -Os \
    -ffreestanding \
    -fdata-sections \
    -ffunction-sections \
    -nostdlib \
    -Wl,--gc-sections \
    -Wl,-T,linker.ld \
    main.c \
    -o firmware.elf

arm-none-eabi-objcopy -O binary firmware.elf firmware.bin
sha256sum firmware.bin
