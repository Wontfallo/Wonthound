# WontHound — ESP32-S3 2.8" (ES3C28P/ES3N28P) — standalone

**Self-contained, copy-anywhere build** of WontHound for the **ESP32-S3** 2.8" IPS
ILI9341V board with **FT6336 capacitive touch** and 16MB QIO flash. Copy this whole
folder anywhere and you have everything needed to build and flash another S3.

- **Build target:** `esp32-s3-es3c28p` (`-DESP32S3_ES3C28P=1`)
- **MCU:** ESP32-S3, native USB-CDC (enumerates as a COM port — no CH340)
- **Pins:** LCD CS=10 DC=46 BL=45 SCK=12 MOSI=11 MISO=13; touch FT6336 I2C
  SCL=15 SDA=16 INT=17 RST=18; NRF24 on P3 SPI + GPIO43/44 (CE/IRQ)
- **UI:** scrollable app-grid home over the custom background, transparent color
  tile icons, button-style submenus, new splash. (No Valhalla/blue-team mode.)

## Build from source (recommended)

Needs [PlatformIO Core](https://platformio.org). On the **first** build it downloads
the ESP32-S3 toolchain and the libraries in `lib_deps` (internet required once);
after that it's offline. From **this folder**:

```sh
pio run -e esp32-s3-es3c28p              # build
pio run -e esp32-s3-es3c28p -t upload    # build + flash (set upload_port first)
pio device monitor -b 115200             # serial monitor
```

Everything is local: `src_dir = .`, the board is a built-in PlatformIO board (no
custom JSON), TFT_eSPI is configured by the local `User_Setup.h` (`-include`), and
the `FT6336`/`TAMC_GT911` touch drivers live in `lib/`. Set `upload_port` in
`platformio.ini` (default `COM10`) to your board's port.

## Flash prebuilt images (no toolchain needed)

`flash/` holds a current v3.5.1 build. Easiest — run the included script:

```powershell
cd flash
.\flash_s3.ps1                 # auto-download reset, flashes COM10
.\flash_s3.ps1 -Port COM7      # specify your port
```

Manual, single-file image:

```sh
esptool.py --chip esp32s3 --port COMx --baud 921600 \
  --before default_reset --after hard_reset \
  write_flash --flash_mode keep --flash_freq keep --flash_size keep \
  0x0 flash/WontHound-S3-ES3C28P-v3.5.1-MERGED.bin
```

Manual, individual parts:

```sh
esptool.py --chip esp32s3 --port COMx --baud 921600 \
  --before default_reset --after hard_reset \
  write_flash --flash_mode keep --flash_freq keep --flash_size keep \
  0x0     flash/bootloader.bin \
  0x8000  flash/partitions.bin \
  0xe000  flash/boot_app0.bin \
  0x10000 flash/firmware.bin
```

> **Important:** use `--flash_mode keep` (don't force `qio`). Re-writing the
> bootloader's flash-mode header on this board causes a boot loop. The S3
> bootloader sits at **0x0** (not 0x1000 like the classic ESP32). If esptool
> can't connect, hold **BOOT** while plugging in, then re-run.

## What's in this folder

- All firmware source (`.ino`, `.cpp`, `.h`), `User_Setup.h`, `lib/` (FT6336 +
  TAMC_GT911 touch drivers).
- `platformio.ini` — pinned to the single `esp32-s3-es3c28p` target.
- `wonthound_bg.h`, `wonthound_icons.h` — generated RGB565 background + menu icons.
- `tools/` — `png_to_rgb565.py` (background) and `icons_to_header.py` (menu icons);
  regenerate the headers from `wonthound-background.png` / `split-icons/` (needs
  Python + Pillow). Only needed if you change the artwork.
- `flash/` — `flash_s3.ps1`, the merged image, the individual `bootloader.bin` /
  `partitions.bin` / `boot_app0.bin` / `firmware.bin`, and `_archive_old/`.

_Note: `boards/nm_cyd_c5.json` is an unrelated leftover and is not used by this build._
</content>
