#!/bin/bash
# Take a screenshot from the Attaky Core ST7789 display via LVGL snapshot.
# The Core's CH340X USB-UART is /dev/cu.usbserial-* (macOS) / /dev/ttyUSB0
# (Linux). Output dimensions are read from the device (currently 320x240).
# Usage: ./screenshot.sh [output.png] [port]

OUTPUT="${1:-screenshot.png}"
if [ -z "$2" ]; then
    case "$(uname -s)" in
        Darwin) PORT=$(ls /dev/cu.usbserial-* 2>/dev/null | head -1) ;;
        *)      PORT="/dev/ttyUSB0" ;;
    esac
    if [ -z "$PORT" ]; then
        echo "Error: no serial port found; pass one explicitly as arg 2." >&2
        exit 1
    fi
else
    PORT="$2"
fi

# Use pio's bundled python if pyserial isn't on the system python.
PY="python3"
if ! python3 -c "import serial" 2>/dev/null; then
    if [ -x "$HOME/.platformio/penv/bin/python" ]; then
        PY="$HOME/.platformio/penv/bin/python"
    fi
fi

TMPBASE=$(mktemp /tmp/screenshot_XXXXXX)
TMPRAW="$TMPBASE.raw"
TMPDIMS="$TMPBASE.dims"
trap "rm -f '$TMPBASE' '$TMPRAW' '$TMPDIMS' '$TMPBASE.png'" EXIT

echo "Taking screenshot from $PORT..."

"$PY" - "$PORT" "$TMPRAW" "$TMPDIMS" << 'PYEOF'
import serial, sys

port_path, raw_path, dims_path = sys.argv[1], sys.argv[2], sys.argv[3]

import time

# Opening the CH340X port toggles DTR/RTS, which auto-resets the ESP32.
# Wait for the firmware to finish booting, then send the command. Retry a
# few times in case the first request lands mid-boot.
port = serial.Serial(port_path, 115200, timeout=3)
# The CH340X auto-program circuit (DTR->EN, RTS->IO0) can hold the ESP32
# in reset on plain open. Pulse a clean reset, then let it boot.
port.setDTR(False); port.setRTS(True)
time.sleep(0.1)
port.setRTS(False)
time.sleep(2.5)            # let the firmware boot
port.reset_input_buffer()

w = h = raw_size = None
for attempt in range(6):   # ~18s worst case
    port.reset_input_buffer()
    port.write(b"screenshot\n")
    port.flush()
    deadline = time.time() + 3
    while time.time() < deadline:
        line = port.readline().decode("utf-8", errors="replace").strip()
        if line.startswith("SCREENSHOT_START"):
            try:
                parts = line.split()
                w, h, raw_size = int(parts[1]), int(parts[2]), int(parts[3])
            except (IndexError, ValueError):
                w = h = raw_size = None   # malformed line → retry
                continue
            break
        if line == "SCREENSHOT_ERR":
            print("Device reported screenshot error", file=sys.stderr)
            sys.exit(1)
    if raw_size is not None:
        break

if raw_size is None:
    print("Timeout: no SCREENSHOT_START after 6 attempts", file=sys.stderr)
    sys.exit(1)
port.timeout = 10

data = b""
while len(data) < raw_size:
    chunk = port.read(min(4096, raw_size - len(data)))
    if not chunk:
        print(f"Timeout: got {len(data)} of {raw_size} bytes", file=sys.stderr)
        sys.exit(1)
    data += chunk

with open(raw_path, "wb") as f:
    f.write(data)
with open(dims_path, "w") as f:
    f.write(f"{w}x{h}\n")

for _ in range(10):
    line = port.readline().decode("utf-8", errors="replace").strip()
    if line == "SCREENSHOT_END":
        break

port.close()
print(f"Captured {w}x{h} ({len(data)} bytes)")
PYEOF

if [ $? -ne 0 ]; then
    echo "Screenshot capture failed"
    exit 1
fi

DIMS=$(cat "$TMPDIMS")
TMPPNG="$TMPBASE.png"
if ffmpeg -y -f rawvideo -pixel_format rgb565le -video_size "$DIMS" \
        -i "$TMPRAW" -update 1 -frames:v 1 "$TMPPNG" 2>/dev/null \
        && [ -s "$TMPPNG" ]; then
    mv "$TMPPNG" "$OUTPUT"
    echo "Saved: $OUTPUT ($DIMS)"
else
    rm -f "$TMPPNG"
    echo "Error: ffmpeg conversion failed (stale $OUTPUT left untouched)"
    exit 1
fi
