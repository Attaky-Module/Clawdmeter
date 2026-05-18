#!/bin/bash
# Build and flash firmware on macOS (Attaky Core_ESP32_1.0).
# The Core uses an external CH340X USB-UART, which enumerates as
# /dev/cu.usbserial-* (NOT /dev/cu.usbmodem*).
# Usage:
#   ./flash-mac.sh                       # auto-detect /dev/cu.usbserial-*
#   ./flash-mac.sh /dev/cu.usbserial-2120  # explicit CH340X serial port
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="$1"

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.usbserial-* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "Error: no /dev/cu.usbserial-* device found. Plug in via USB-C"
        echo "and check the CH340 driver is installed."
        exit 1
    fi
fi

if ! command -v pio >/dev/null; then
    echo "Error: 'pio' not found. Install with:"
    echo "  brew install platformio"
    exit 1
fi

echo "=== Flashing firmware (Attaky Core) ==="
echo "Port: $PORT"
echo ""

cd "$SCRIPT_DIR/firmware"
pio run -t upload --upload-port "$PORT"

echo ""
echo "=== Done ==="
echo "Monitor with: pio device monitor -p $PORT -b 115200"
