param(
    [string]$Pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

if (-not (Test-Path -LiteralPath $Pio)) {
    throw "PlatformIO executable not found: $Pio"
}

# The nm-cyd-c5 env pins the pioarduino platform 55.03.38, which bundles the
# Arduino core 3.3.8 + ESP32-C5 toolchain. No separate framework install step is
# needed; PlatformIO resolves everything from the platform package.
Push-Location $projectRoot
try {
    $env:PYTHONIOENCODING = "utf-8"

    & $Pio pkg install -e nm-cyd-c5
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $Pio run -e nm-cyd-c5 -j 1
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
