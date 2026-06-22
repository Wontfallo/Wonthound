# Codex handoff: C5 TFT build pipeline

Use PlatformIO for the ESP32-C5 TFT/WontHound work, with the Arduino framework.
Do not try to make this an Arduino IDE-only build.

Why: in the WontWiFi analyzer project, the plain ESP32-S3/headless firmware was
built with arduino-cli, but the ESP32-C5 work moved to a dedicated PlatformIO
project because C5 support needed the pioarduino ESP32 platform, a custom board
JSON, and toolchain patches.

Reference pattern from:
C:\Users\WontML\dev\embedded_esp\esp32-S3-WontWiFi-analyzer\firmware\wontwifi-c5-pio\platformio.ini

Core env:

```ini
[platformio]
src_dir = src

[env:nm-cyd-c5]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.38/platform-espressif32.zip
board = nm_cyd_c5
framework = arduino
board_build.partitions = boards/nm_cyd_c5_partitions.csv
board_build.flash_mode = dio
board_build.f_flash = 80000000L
board_build.arduino.memory_type = dio_qspi
board_upload.flash_size = 16MB
upload_speed = 115200
monitor_speed = 115200
```

Expected local support files:

- `boards/nm_cyd_c5.json`
- `boards/nm_cyd_c5_partitions.csv`
- `scripts/patch_esptool_c5.py`
- possibly `scripts/patch_tft_espi_c5.py` or other display/library patching
- saved PowerShell helpers like `scripts/build_nm_cyd_c5.ps1`,
  `scripts/upload_nm_cyd_c5.ps1`, and `scripts/flash_monitor_nm_cyd_c5.ps1`

Build shape:

```powershell
pio pkg install
pio run -e nm-cyd-c5 -j1
```

If building from a parent project, make sure the parent `platformio.ini` actually
has `[env:nm-cyd-c5]`; the WIP README says that env was removed from the parent
file even though stale `.pio/build/nm-cyd-c5/` output may still exist.

Gotchas to keep straight:

- C5 TFT/WontHound is separate from the plain ESP32-S3 headless firmware.
- In WontWiFi, S3 source build used `arduino-cli compile` with an ESP32-S3 FQBN.
  That does not mean the C5 TFT port should use Arduino CLI.
- The C5 standalone port is separate from any "C5 co-processor over UART" code.
  Do not merge those concepts.
- Display and touch bring-up were the hard unfinished parts. Check logs named
  like `c5_*orientation*`, `*panel_probe*`, and `*touch*` before changing pins.
- Keep capability reporting honest: do not claim C5-only, S3-only, TFT, touch,
  GPS, or radio features until the firmware proves them at runtime.

WebUI side, if copied from WontWiFi:

- app pipeline was `pnpm install`, `pnpm dev`, `pnpm build`, `pnpm run check`
- browser talks to firmware over WebSerial at 115200 baud
- firmware should expose proof commands like `INFO`, `CAPABILITIES`, `PROTOCOL`,
  and `COMMANDS` so the UI can verify the target instead of guessing
