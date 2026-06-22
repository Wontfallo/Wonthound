param(
    [string]$Port = "",
    [int]$WaitSeconds = 120,
    [switch]$Monitor,
    [int]$CaptureSeconds = 0,
    [string]$LogPath = "",
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
            return [pscustomobject]@{
                Port = $Matches[0].Trim("(", ")")
                FriendlyName = $candidate.FriendlyName
                InstanceId = $candidate.InstanceId
            }
        }
    }

    return $null
}

function Wait-C5UploadPort {
    param([int]$TimeoutSeconds)

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $candidate = Get-C5UploadPort
        if ($null -ne $candidate) {
            return $candidate
        }

        $remaining = [Math]::Max(0, [int]($deadline - (Get-Date)).TotalSeconds)
        Write-Host ("Waiting for ESP32-C5 USB serial/JTAG port... {0}s remaining" -f $remaining)
        Start-Sleep -Seconds 2
    }

    return $null
}

function Invoke-C5MonitorCapture {
    param(
        [string]$Port,
        [int]$Seconds,
        [string]$OutputPath
    )

    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $OutputPath = Join-Path $projectRoot ("logs\nm-cyd-c5-runtime-{0}.log" -f $stamp)
    }

    $logDir = Split-Path -Parent $OutputPath
    if (-not [string]::IsNullOrWhiteSpace($logDir) -and -not (Test-Path -LiteralPath $logDir)) {
        New-Item -ItemType Directory -Path $logDir | Out-Null
    }

    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $Pio
    $psi.ArgumentList.Add("device")
    $psi.ArgumentList.Add("monitor")
    $psi.ArgumentList.Add("-e")
    $psi.ArgumentList.Add("nm-cyd-c5")
    $psi.ArgumentList.Add("--port")
    $psi.ArgumentList.Add($Port)
    $psi.ArgumentList.Add("--baud")
    $psi.ArgumentList.Add("115200")
    $psi.ArgumentList.Add("--quiet")
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $psi
    $buffer = New-Object System.Text.StringBuilder

    Write-Host ("Capturing serial monitor for {0}s to {1}" -f $Seconds, $OutputPath)
    [void]$process.Start()
    Start-Sleep -Seconds $Seconds

    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }

    [void]$buffer.Append($process.StandardOutput.ReadToEnd())
    [void]$buffer.Append($process.StandardError.ReadToEnd())
    $buffer.ToString() | Set-Content -LiteralPath $OutputPath -Encoding UTF8

    if ($buffer.ToString() -match "\[C5-WIFI\]") {
        Write-Host "Captured C5 WiFi runtime report."
    } else {
        Write-Host "No [C5-WIFI] runtime report was captured in the timed window."
    }

    return $OutputPath
}

Push-Location $projectRoot
try {
    if ([string]::IsNullOrWhiteSpace($Port)) {
        $detected = Wait-C5UploadPort -TimeoutSeconds $WaitSeconds
        if ($null -eq $detected) {
            throw "No ESP32-C5 USB serial/JTAG COM port appeared within $WaitSeconds seconds."
        }

        $Port = $detected.Port
        Write-Host ("Detected {0}: {1}" -f $Port, $detected.FriendlyName)
    }

    $env:PYTHONIOENCODING = "utf-8"
    $env:PYTHONUTF8 = "1"

    & $Pio run -e nm-cyd-c5 -j 1
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $Python $Esptool --chip esp32c5 --port $Port --baud $Baud --before default-reset --after no-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x10000 .\.pio\build\nm-cyd-c5\firmware.bin
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Flash complete. ESP32-C5 is intentionally left in bootloader; unplug/replug to boot the new app."

    if ($CaptureSeconds -gt 0) {
        Write-Host "Waiting 8 seconds before capture. If the board does not auto-boot, unplug/replug it now."
        Start-Sleep -Seconds 8
        $capturedLog = Invoke-C5MonitorCapture -Port $Port -Seconds $CaptureSeconds -OutputPath $LogPath
        Write-Host ("Runtime capture saved: {0}" -f $capturedLog)
        exit 0
    }

    if ($Monitor) {
        Write-Host "Opening serial monitor. Watch for [C5-WIFI] runtime report."
        & $Pio device monitor -e nm-cyd-c5 --port $Port --baud 115200
        exit $LASTEXITCODE
    }

    Write-Host "Re-run with -Monitor or -CaptureSeconds 20 after unplug/replug to capture the [C5-WIFI] runtime report."
    exit 0
}
finally {
    Pop-Location
}
