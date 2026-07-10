from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def test_attaky_core_platformio_env_selects_board_folder():
    text = (ROOT / "firmware/platformio.ini").read_text()
    assert "[env:attaky_core_esp32]" in text
    block = text.split("[env:attaky_core_esp32]", 1)[1].split("\n[env:", 1)[0]
    assert "+<boards/attaky_core_esp32/>" in block
    assert "-DARDUINO_USB_CDC_ON_BOOT=1" not in block
    assert "upload_speed = 460800" in block
    assert "bodmer/TFT_eSPI" in block


def test_attaky_core_board_hal_files_exist():
    board = ROOT / "firmware/src/boards/attaky_core_esp32"
    for name in [
        "board.h",
        "board_init.cpp",
        "caps.cpp",
        "display.cpp",
        "input.cpp",
        "power.cpp",
        "touch.cpp",
        "imu.cpp",
        "sound.cpp",
        "io_expander.h",
        "io_expander.cpp",
    ]:
        assert (board / name).is_file(), name


def test_attaky_core_preserves_extended_hid_buttons():
    input_hal = (ROOT / "firmware/src/hal/input_hal.h").read_text()
    for token in [
        "INPUT_BTN_UP",
        "INPUT_BTN_DOWN",
        "INPUT_BTN_LEFT",
        "INPUT_BTN_RIGHT",
        "INPUT_BTN_SELECT",
        "INPUT_BTN_L1",
        "INPUT_BTN_R2",
        "INPUT_BTN_UI_NAV",
    ]:
        assert token in input_hal

    main = (ROOT / "firmware/src/main.cpp").read_text()
    for keycode in [
        "0x52",  # Up Arrow
        "0x51",  # Down Arrow
        "0x50",  # Left Arrow
        "0x4F",  # Right Arrow
        "0x28",  # Enter
        "0x2B",  # Tab
        "0x2C",  # Space
    ]:
        assert keycode in main


def test_attaky_core_keeps_attaky_ble_name():
    ble = (ROOT / "firmware/src/ble.cpp").read_text()
    caps = (ROOT / "firmware/src/boards/attaky_core_esp32/caps.cpp").read_text()
    daemon = (ROOT / "daemon/claude_usage_daemon.py").read_text()
    linux_daemon = (ROOT / "daemon/claude-usage-daemon.sh").read_text()
    windows_daemon = (ROOT / "daemon/claude_usage_daemon_windows.py").read_text()
    assert '.ble_name = "Attaky Claude Monitor"' in caps
    assert 'board_caps().ble_name' in ble
    assert 'DEVICE_NAME = os.environ.get("CLAWDMETER_DEVICE_NAME", "Attaky Claude Monitor")' in daemon
    assert 'DEVICE_NAME="${CLAWDMETER_DEVICE_NAME:-Attaky Claude Monitor}"' in linux_daemon
    assert 'DEVICE_NAME = os.environ.get("CLAWDMETER_DEVICE_NAME", "Attaky Claude Monitor")' in windows_daemon


def test_attaky_core_has_320x240_layout_guard():
    ui = (ROOT / "firmware/src/ui.cpp").read_text()
    assert "c.width == 320 && c.height == 240" in ui
    assert "Compact landscape layout -- tuned for Attaky Core 320x240" in ui
    assert "L.margin = 8;" in ui
    assert "L.content_y = 44;" in ui
    assert "L.usage_panel_h = 78;" in ui
    assert "L.usage_panel_gap = 8;" in ui


if __name__ == "__main__":
    for test in [
        test_attaky_core_platformio_env_selects_board_folder,
        test_attaky_core_board_hal_files_exist,
        test_attaky_core_preserves_extended_hid_buttons,
        test_attaky_core_keeps_attaky_ble_name,
        test_attaky_core_has_320x240_layout_guard,
    ]:
        test()
    print("attaky core port static tests passed")
