#!/bin/bash
# Build and flash firmware on Linux (Attaky Core_ESP32_1.0).
# The Core's CH340X USB-UART enumerates as /dev/ttyUSB0 (not ttyACM0).
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-/dev/ttyUSB0}"

echo "=== Flashing firmware (Attaky Core) ==="
echo "Port: $PORT"
echo ""

cd "$SCRIPT_DIR/firmware"
~/.platformio/penv/bin/pio run -t upload --upload-port "$PORT"

echo ""
echo "=== Done! ==="
