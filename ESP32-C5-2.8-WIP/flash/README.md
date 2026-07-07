# WontHound-CYD — NM-CYD-C5 flash images (ESP32-C5, v3.5.1)

Dual-band Wi-Fi 6 (2.4 + 5 GHz), 2.8" 240×320 ST7789 + XPT2046 resistive touch.

## Easiest: web flasher (single merged file)
Flash **`WontHound-C5-NM-CYD-v3.5.1-MERGED.bin`** at offset **`0x0`**.
- **ESP Web Tools:** point it at `manifest.json` (chipFamily `ESP32-C5`, one part at offset 0).
- **esptool-js / custom flasher:** pick the `*-MERGED.bin` and write it at `0x0`.

The merged image already contains the bootloader at its `0x2000` offset, so writing the
whole blob at `0x0` is correct — do **not** write it at `0x1000`/`0x2000`.

## esptool (CLI)
Merged image:
```
esptool --chip esp32c5 --port <COM> --baud 460800 write_flash 0x0 WontHound-C5-NM-CYD-v3.5.1-MERGED.bin
```
Individual parts (only if you are not using the merged image):
| Offset   | File            |
|----------|-----------------|
| `0x2000` | `bootloader.bin` |
| `0x8000` | `partitions.bin` |
| `0xe000` | `boot_app0.bin`  |
| `0x10000`| `firmware.bin`  (identical to `WontHound-C5-NM-CYD-v3.5.1-app.bin`) |

## Notes
- **ESP32-C5 second-stage bootloader lives at `0x2000`** (not `0x0`/`0x1000` like ESP32/S3/C3).
- Writing the **MERGED / full-parts** image erases NVS, which includes the **touch calibration** —
  you'll recalibrate once on first boot. Writing **only** `firmware.bin` at `0x10000` preserves it.
- If esptool can't connect, hold **BOOT** while plugging in, then retry.

## Files
- `WontHound-C5-NM-CYD-v3.5.1-MERGED.bin` — full image, flash at `0x0` (web flasher / single-shot).
- `WontHound-C5-NM-CYD-v3.5.1-app.bin` / `firmware.bin` — app only, flash at `0x10000` (keeps calibration).
- `bootloader.bin`, `partitions.bin`, `boot_app0.bin` — raw parts for manual/partial flashing.
- `manifest.json` — ESP Web Tools manifest.
- `flash_c5.ps1` — local PowerShell flasher (esptool-based).
