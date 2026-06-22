param(
    [string]$Port = "",
    [int]$Baud = 921600,
    [string]$Pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe",
    [string]$Python = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe",
    [string]$Esptool = "$env:USERPROFILE\.platformio\packages\tool-esptoolpy\esptool.py"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

if (-not (Test-Path -LiteralPath $Pio)) {
    throw "PlatformIO executable not found: $Pio"
}
if (-not (Test-Path -LiteralPath $Python)) {
    throw "PlatformIO Python executable not found: $Python"
}
if (-not (Test-Path -LiteralPath $Esptool)) {
    throw "esptool.py not found: $Esptool"
}

function Get-C5UploadPort {
    $ports = Get-PnpDevice -Class Ports -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object {
            $_.InstanceId -match "VID_303A|VID_10C4|VID_1A86|VID_0403" -or
            $_.FriendlyName -match "Espressif|ESP|USB Serial|CP210|CH340|UART|JTAG"
        }

    foreach ($candidate in $ports) {
        if ($candidate.FriendlyName -match "\(COM\d+\)") {
            return $Matches[0].Trim("(", ")")
        }
    }

    return $null
}

Push-Location $projectRoot
try {
    if ([string]::IsNullOrWhiteSpace($Port)) {
        $Port = Get-C5UploadPort
    }

    if ([string]::IsNullOrWhiteSpace($Port)) {
        throw "No ESP32-C5 USB serial/JTAG COM port found. Windows currently exposes no Espressif/CP210/CH340/FTDI port."
    }

    $env:PYTHONIOENCODING = "utf-8"
    $env:PYTHONUTF8 = "1"

    & $Pio run -e nm-cyd-c5 -j 1
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $Python $Esptool --chip esp32c5 --port $Port --baud $Baud --before default-reset --after no-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x10000 .\.pio\build\nm-cyd-c5\firmware.bin
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Flash complete. ESP32-C5 is intentionally left in bootloader; unplug/replug to boot the new app."
    exit 0
}
finally {
    Pop-Location
}
