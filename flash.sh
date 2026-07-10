#!/bin/bash
# Build and flash Clawdmeter firmware on Linux.
# Usage:
#   ./flash.sh <board>                  # default port /dev/ttyACM0
#   ./flash.sh <board> /dev/ttyACM1     # explicit USB serial port
#
# <board> is the PlatformIO env name, e.g. waveshare_amoled_216 or waveshare_amoled_18.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BOARD="$1"
# Native-USB boards enumerate as ttyACM*; CH340-bridged boards (Attaky
# Core) as ttyUSB*. Default to whichever is present, preferring ttyACM0.
if [ -n "${2:-}" ]; then
    PORT="$2"
elif [ -e /dev/ttyACM0 ]; then
    PORT=/dev/ttyACM0
elif [ -e /dev/ttyUSB0 ]; then
    PORT=/dev/ttyUSB0
else
    PORT=/dev/ttyACM0
fi

if [ -z "$BOARD" ]; then
    echo "Error: board env name is required."
    echo "Usage: $0 <board> [port]"
    echo "Available boards:"
    grep -E '^\[env:' "$SCRIPT_DIR/firmware/platformio.ini" | sed 's/\[env:/  /;s/\]//'
    exit 1
fi

echo "=== Flashing Clawdmeter ==="
echo "Board: $BOARD"
echo "Port:  $PORT"
echo ""

cd "$SCRIPT_DIR/firmware"
~/.platformio/penv/bin/pio run -e "$BOARD" -t upload --upload-port "$PORT"

echo ""
echo "=== Done! ==="
