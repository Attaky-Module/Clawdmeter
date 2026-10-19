"""Static guards for the Attaky Core port (env, board folder, BLE name,
pairing on BOOT, extended keys). Stdlib only:

    python3 -m unittest discover -s tools/ci/tests -v
"""

import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
BOARD = REPO / "firmware/src/boards/attaky_core_esp32"


def env_block(name: str) -> str:
    text = (REPO / "firmware/platformio.ini").read_text()
    return text.split(f"[env:{name}]", 1)[1].split("\n[env:", 1)[0]


class AttakyCorePortTest(unittest.TestCase):
    def test_env_selects_only_the_board_folder(self):
        block = env_block("attaky_core_esp32")
        self.assertIn("-<boards/>", block)
        self.assertIn("+<boards/attaky_core_esp32/>", block)
        self.assertIn('-DCLAWD_ENV=\\"${this.__env__}\\"', block)
        # CH340 bridge: no native USB CDC.
        self.assertNotIn("ARDUINO_USB_CDC_ON_BOOT", block)
        self.assertIn("upload_speed = 460800", block)

    def test_env_is_reproducible(self):
        block = env_block("attaky_core_esp32")
        self.assertIn("-ffile-prefix-map=${sysenv.HOME}=.", block)
        deps = block.split("lib_deps =", 1)[1].strip().splitlines()
        deps = [d.strip() for d in deps if d.strip() and not d.strip().startswith(";")]
        self.assertTrue(deps)
        for dep in deps:
            self.assertRegex(dep, r"^[\w./ -]+@\d+\.\d+\.\d+$", "lib_deps must be exact versions")

    def test_env_is_built_by_default(self):
        text = (REPO / "firmware/platformio.ini").read_text()
        default_envs = text.split("default_envs =", 1)[1].split("\n[", 1)[0]
        self.assertIn("attaky_core_esp32", default_envs.split())

    def test_board_hal_files_exist(self):
        for name in ["board.h", "board_init.cpp", "caps.cpp", "display.cpp", "input.cpp",
                     "power.cpp", "touch.cpp", "imu.cpp", "sound.cpp", "io_expander.h",
                     "io_expander.cpp", "CLAUDE.md"]:
            self.assertTrue((BOARD / name).is_file(), name)

    def test_ble_name_and_boot_pairing(self):
        caps = (BOARD / "caps.cpp").read_text()
        self.assertIn('.ble_name = "Attaky Claude Monitor"', caps)
        self.assertIn(".pair_on_boot = true", caps)
        self.assertIn(".has_pwr_button = true", caps)
        ble = (REPO / "firmware/src/ble.cpp").read_text()
        self.assertIn("board_caps().ble_name", ble)
        self.assertNotRegex(ble, r"\bDEVICE_NAME\b")
        self.assertIn("PAIR_TEXT_BOOT", (REPO / "firmware/src/ui.cpp").read_text())
        self.assertIn("PAIR_TEXT_BOOT", (REPO / "firmware/src/layout.cpp").read_text())

    def test_extended_keys(self):
        input_hal = (REPO / "firmware/src/hal/input_hal.h").read_text()
        for token in ["INPUT_BTN_UP", "INPUT_BTN_DOWN", "INPUT_BTN_LEFT", "INPUT_BTN_RIGHT",
                      "INPUT_BTN_SELECT", "INPUT_BTN_UI_NAV", "INPUT_BTN_COUNT"]:
            self.assertIn(token, input_hal)
        main = (REPO / "firmware/src/main.cpp").read_text()
        for keycode in ["0x52", "0x51", "0x50", "0x4F", "0x28"]:
            self.assertIn(keycode, main)
        # Only hid_release() may clear the shared HID report.
        self.assertEqual(len(re.findall(r"ble_keyboard_release\(\);", main)), 1)

    def test_manifest_and_app_know_the_board(self):
        self.assertIn('"attaky_core_esp32"', (REPO / "tools/ci/make_manifest.py").read_text())
        boards = (REPO / "desktop/crates/clawdmeter-app/src/boards.rs").read_text()
        self.assertIn('env: "attaky_core_esp32"', boards)
        self.assertTrue((REPO / "desktop/crates/clawdmeter-app/assets/boards/attaky_core_esp32.svg").is_file())


if __name__ == "__main__":
    unittest.main()
