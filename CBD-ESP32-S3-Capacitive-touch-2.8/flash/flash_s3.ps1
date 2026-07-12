<#
  Flash WontHound to an ESP32-S3 ES3C28P (2.8" IPS + FT6336 cap touch, 16MB).

  Usage (PowerShell):
      .\flash_s3.ps1                # auto-detects esptool, flashes COM10
      .\flash_s3.ps1 -Port COM7     # specify the board's COM port

  No BOOT button needed — the script uses the USB-serial-JTAG auto-download
  reset. If it can't connect, hold BOOT while plugging the board in, then re-run.

  Flashes the individual images with their original headers preserved
  (--flash_mode keep), which is the combination confirmed working on hardware.
  (A single-file image, WontHound-S3-ES3C28P-v3.5.1-MERGED.bin, is also in this
  folder if you prefer one-shot flashing at 0x0.)
#>
param([string]$Port = "COM10")

$here = Split-Path -Parent $MyInvocation.MyCommand.Path

# Find esptool: PlatformIO's bundled copy first, then PATH.
$esptool = "$env:USERPROFILE\.platformio\penv\Scripts\esptool.exe"
if (-not (Test-Path $esptool)) { $esptool = "esptool.py" }

Write-Host "Flashing ESP32-S3 on $Port ..." -ForegroundColor Cyan
& $esptool --chip esp32s3 --port $Port --baud 921600 `
    --before default_reset --after hard_reset `
    write_flash --flash_mode keep --flash_freq keep --flash_size keep -z `
    0x0     "$here\bootloader.bin" `
    0x8000  "$here\partitions.bin" `
    0xe000  "$here\boot_app0.bin" `
    0x10000 "$here\firmware.bin"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Done. The board should reboot into WontHound." -ForegroundColor Green
} else {
    Write-Host "Flash failed. Try: hold BOOT while plugging in, then re-run with the right -Port." -ForegroundColor Yellow
}
