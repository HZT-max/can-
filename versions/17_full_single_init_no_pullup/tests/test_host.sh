#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

gcc \
    -std=c11 \
    -O2 \
    -Wall \
    -Wextra \
    -Werror \
    -DCANARD_ENABLE_TAO_OPTION=1 \
    -Ilibcanard \
    -Idsdl \
    tests/test_dronecan_roundtrip.c \
    libcanard/canard.c \
    dsdl/uavcan.tunnel.Protocol.c \
    dsdl/uavcan.tunnel.Targetted.c \
    -o tests/test_dronecan_roundtrip

./tests/test_dronecan_roundtrip
