# CYD-ESP32-3E — Classic Cheap Yellow Display (Freenove ESP32-32E / ESP32-2432S028, 2.8")

Self-contained, copy-anywhere build of WontHound for the classic **2.8" 240x320
ILI9341 CYD** with **XPT2046 resistive touch** — specifically the **Freenove
ESP32-32E** with an **NRF24L01+PA+LNA** on the P3/P4 SPI breakout (+ botch wire)
and a GPS module. Copy this whole folder anywhere and you have everything needed
to build and flash another one.

- **Build target:** `esp32-cyd` (board `esp32dev`, with `-DFREENOVE_E32R28T_P3_NRF24`).
- **MCU:** ESP32 (dual-core, CH340 USB-serial — enumerates as a COM port).
- **Display/touch:** ILI9341 240x320, XPT2046 resistive, backlight GPIO21.
  Panel inversion is **ON** (this CYD panel shows negative colors otherwise).
- **UI:** fixed single-screen "dashboard" home — 7 tool icons around the central
  bulb + a bottom utility bar (Tools/Setting/About). No scrolling = no flicker on
  the PSRAM-less classic ESP32; every icon is a clean fixed tap target. No
  disclaimer screen.

### Radio / GPS wiring (Freenove ESP32-32E)

| Signal | GPIO | Notes |
|--------|------|-------|
| NRF24 SCK / MOSI / MISO | 18 / 23 / 19 | shared VSPI bus |
| NRF24 CSN | 27 | P3 SPI_CS (shared with CC1101 CS) |
| NRF24 CE  | 16 | IO16 RGB pad → botch wire to P4 pin 3 |
| NRF24 IRQ | — | unused (freed for GPS) |
| GPS RX (ESP32 receives) | 35 | P4 IO35, input-only, 9600 baud |

## Build & flash from source (recommended — always current)

PlatformIO handles the flash offsets automatically. On the **first** build it
downloads the ESP32 toolchain and the `lib_deps` libraries (internet required
once); after that it's offline.

```sh
# from this folder
pio run -e esp32-cyd                 # build
pio run -e esp32-cyd -t upload       # build + flash (set upload_port first)
pio device monitor -b 115200         # serial monitor
```

Everything is local: `src_dir = .`, the board is a built-in PlatformIO board (no
custom JSON), TFT_eSPI is configured by the local `User_Setup.h` (`-include`).
Set `upload_port = COMx` in `platformio.ini` for your board.

## Flash prebuilt binaries (no toolchain needed)

`flash/` holds a current build. Easiest — run the included script:

```powershell
cd flash
.\flash_3e.ps1                 # auto-download reset, flashes COM28
.\flash_3e.ps1 -Port COM5      # specify your port
```

Single merged image (one-shot @ 0x0):

```sh
esptool.py --chip esp32 --port COMx --baud 460800 \
  write_flash --flash_mode keep --flash_freq keep --flash_size keep \
  0x0 flash/WontHound-CYD-v3.5.1-MERGED.bin
```

Individual parts:

```sh
esptool.py --chip esp32 --port COMx --baud 460800 \
  write_flash --flash_mode keep --flash_freq keep --flash_size keep \
  0x1000  flash/bootloader.bin \
  0x8000  flash/partitions.bin \
  0xe000  flash/boot_app0.bin \
  0x10000 flash/firmware.bin
```

> The classic ESP32 bootloader sits at **0x1000** (not 0x0 like the S3). Use
> `--flash_mode keep` (don't force the header). If esptool can't connect, hold
> **BOOT** while plugging in, then re-run.

## What's in this folder

- All firmware source (`.ino`, `.cpp`, `.h`), `User_Setup.h`, `lib/`, `boards/`.
- `platformio.ini` — pinned to the single `esp32-cyd` target (Freenove NRF flag).
- `ui/` — source artwork: `background-image.png` + `50px icons/` (the menu icons).
- `tools/` — `png_to_rgb565.py` (background) and `icons_to_header_vivid.py` (menu
  icons; brightness/saturation boosted for this lower-contrast panel). Regenerate
  `wonthound_bg.h` / `wonthound_icons.h` from `ui/` only if you change the art.
- `flash/` — `flash_3e.ps1`, the merged image, the individual `bootloader.bin` /
  `partitions.bin` / `boot_app0.bin` / `firmware.bin`, and `_archive_old/`.
