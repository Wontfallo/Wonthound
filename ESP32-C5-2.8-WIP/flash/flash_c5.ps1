<#
  Flash WontHound to the NM-CYD-C5 (ESP32-C5, 2.8" 240x320 ST7789 + XPT2046
  resistive touch, Wi-Fi 6 dual-band 2.4/5 GHz).

  Usage (PowerShell):
      .\flash_c5.ps1                 # auto-detects the C5 USB-serial port
      .\flash_c5.ps1 -Port COM12     # or specify it explicitly

  ESP32-C5 (RISC-V): the second-stage bootloader sits at 0x2000 (NOT 0x0 or
  0x1000 like older ESP32/S3 parts). Flashing it anywhere else boot-loops the
  chip with "invalid header" / ets_flash_boot errors. Images are flashed with
  explicit dio / 80 MHz / 16 MB to match the build. If esptool can't connect,
  hold the BOOT button while plugging the board in, then re-run.

  A one-shot image, WontHound-C5-NM-CYD-v3.5.1-MERGED.bin, is also here and can
  be flashed alone at 0x0 (see the -Merged switch).
#>
param(
    [string]$Port = "",
    [switch]$Merged
)

# esptool v5 draws a Unicode progress bar that crashes on the Windows cp1252
# console; force UTF-8 so the flash completes.
$env:PYTHONIOENCODING = "utf-8"
$env:PYTHONUTF8 = "1"

$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$esptool = "$env:USERPROFILE\.platformio\penv\Scripts\esptool.exe"
if (-not (Test-Path $esptool)) { $esptool = "esptool.py" }

# Auto-detect the C5 port (Espressif USB-Serial/JTAG, VID 303A) if not given.
if ([string]::IsNullOrWhiteSpace($Port)) {
    $cand = Get-PnpDevice -Class Ports -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object { $_.InstanceId -match "VID_303A" -and $_.FriendlyName -match "\(COM\d+\)" } |
        Select-Object -First 1
    if ($cand -and $cand.FriendlyName -match "(COM\d+)") { $Port = $Matches[1] }
}
if ([string]::IsNullOrWhiteSpace($Port)) {
    Write-Host "No ESP32-C5 COM port found. Plug the board in (hold BOOT if needed) or pass -Port COMx." -ForegroundColor Yellow
    exit 1
}

Write-Host "Flashing NM-CYD-C5 (ESP32-C5) on $Port ..." -ForegroundColor Cyan

if ($Merged) {
    & $esptool --chip esp32c5 --port $Port --baud 921600 `
        --before default-reset --after hard-reset `
        write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB `
        0x0 "$here\WontHound-C5-NM-CYD-v3.5.1-MERGED.bin"
} else {
    & $esptool --chip esp32c5 --port $Port --baud 921600 `
        --before default-reset --after hard-reset `
        write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB `
        0x2000  "$here\bootloader.bin" `
        0x8000  "$here\partitions.bin" `
        0xe000  "$here\boot_app0.bin" `
        0x10000 "$here\firmware.bin"
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "Done. The board should reboot into WontHound." -ForegroundColor Green
    Write-Host "First boot runs a one-time 4-corner touch calibration - tap each corner." -ForegroundColor Green
} else {
    Write-Host "Flash failed. Try: hold BOOT while plugging in, then re-run with the right -Port." -ForegroundColor Yellow
}
