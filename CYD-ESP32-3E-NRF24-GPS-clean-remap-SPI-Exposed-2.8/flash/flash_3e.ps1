<#
  Flash WontHound to a classic CYD ESP32-2432S028 / Freenove ESP32-32E
  ("3E", 2.8" 240x320 ILI9341 + XPT2046 resistive touch).

  Usage (PowerShell):
      .\flash_3e.ps1                # auto-detects esptool, flashes COM28
      .\flash_3e.ps1 -Port COM5     # specify the board's COM port

  Classic ESP32 (not S3): bootloader sits at 0x1000. Flashes the individual
  images with their original headers preserved (--flash_mode keep), the
  combination confirmed working on hardware. If esptool can't connect, hold the
  BOOT button while plugging the board in, then re-run.
  (A one-shot image, WontHound-CYD-v3.5.1-MERGED.bin, is also here for 0x0.)
#>
param([string]$Port = "COM28")

$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$esptool = "$env:USERPROFILE\.platformio\penv\Scripts\esptool.exe"
if (-not (Test-Path $esptool)) { $esptool = "esptool.py" }

Write-Host "Flashing CYD ESP32 (3E) on $Port ..." -ForegroundColor Cyan
& $esptool --chip esp32 --port $Port --baud 460800 `
    --before default_reset --after hard_reset `
    write_flash --flash_mode keep --flash_freq keep --flash_size keep -z `
    0x1000  "$here\bootloader.bin" `
    0x8000  "$here\partitions.bin" `
    0xe000  "$here\boot_app0.bin" `
    0x10000 "$here\firmware.bin"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Done. The board should reboot into WontHound." -ForegroundColor Green
} else {
    Write-Host "Flash failed. Try: hold BOOT while plugging in, then re-run with the right -Port." -ForegroundColor Yellow
}
