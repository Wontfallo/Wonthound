# ESP32-C5-2.8 — WORK IN PROGRESS (no finished HaleHound port yet)

This folder gathers everything related to the **NM-CYD-C5** (ESP32-C5 2.8" TFT)
so it's all in one place to resume later. **There is no completed HaleHound/
WontHound firmware for the C5 yet** — what exists is build scaffolding,
experiments, vendor reference firmware, and the board datasheet.

## Where you left off (reconstructed from the files)

- You were bringing up the C5 via a dedicated PlatformIO env named **`nm-cyd-c5`**
  that pinned the **pioarduino platform `55.03.38`** (bundles Arduino core 3.3.8
  + the ESP32-C5 toolchain) against the custom board `boards/nm_cyd_c5.json`.
- ⚠️ **That `[env:nm-cyd-c5]` is NO LONGER in the parent `platformio.ini`.** The
  current file has 8 envs, none for the C5. To resume you must re-add it (recipe
  below). A stale `.pio/build/nm-cyd-c5/` build dir still exists in the parent.
- Per your note: you had only used the **C5 radio** driven by a **separate Web-UI**
  experiment — that Web-UI is its own project and was not found in or near this
  repo. The actual *HaleHound port to the C5* is the unfinished part.
- The display/touch bring-up was the active struggle — see the many
  `logs/c5_*orientation*`, `*panel_probe*`, `*touch*` logs.

## How to resume the build

1. Re-add an env to a `platformio.ini` (parent or a copy here), e.g.:

   ```ini
   [env:nm-cyd-c5]
   platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.38/platform-espressif32.zip
   board = nm_cyd_c5            ; uses boards/nm_cyd_c5.json
   framework = arduino
   board_build.partitions = boards/nm_cyd_c5_partitions.csv
   ; build_flags / lib_deps as needed
   ```

2. Build / flash with the saved scripts (paths assume they run from a project root
   that has the `nm-cyd-c5` env and `boards/` def):
   - `scripts/build_nm_cyd_c5.ps1` — `pio pkg install` + `pio run -e nm-cyd-c5 -j1`
   - `scripts/upload_nm_cyd_c5.ps1` — builds, auto-detects the C5 COM port, then
     `esptool --chip esp32c5 ... write-flash 0x10000 firmware.bin`
   - `scripts/flash_monitor_nm_cyd_c5.ps1`, `scripts/audit_nm_cyd_c5.ps1`
   - `scripts/patch_tft_espi_c5.py`, `scripts/patch_esptool_c5.py` — toolchain
     patches that were needed for C5 support.

## Board facts (`boards/nm_cyd_c5.json`)

- MCU `esp32c5`, variant `esp32c5`, 16MB QIO flash @ 80MHz, 240MHz CPU.
- Native USB-CDC (`-DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1`).
- Connectivity: Wi-Fi (incl. 5 GHz), Bluetooth, IEEE 802.15.4.
- Vendor: NM Tech — <https://www.nmminer.com/product/nm-cyd-c5-...>

## Contents

- `scripts/` — C5 build/flash/audit PowerShell + esptool/TFT_eSPI patch scripts.
- `boards/` — `nm_cyd_c5.json` board def + `nm_cyd_c5_partitions.csv`.
- `reference-firmwares/` — **third-party**, for reference/recovery only:
  - `bruce-c5/firmware.bin` — Bruce firmware for C5.
  - `marauder-c5/` — ESP32 Marauder for C5 (full set).
  - `nm-cyd-c5-vendor/` — vendor bootloader/partitions/boot_app0.
- `docs/` — NM-CYD-C5 datasheet PDF + extracted pin/schematic pages.
- `logs/` — the C5 bring-up build/boot/display/touch logs.

## Note on the in-tree "C5 co-processor" code

Separately, `cyd_config.h` in the main firmware has a **"WontHound-Alpha" UART
co-processor link** — that's for using a C5 as a *radio/GPS helper to the S3*
over UART (auto-detected, gated). It is **not** the standalone C5 port and lives
in the shared S3/CYD firmware, not here.
