param(
    [string]$Pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$failures = New-Object System.Collections.Generic.List[string]

function Add-Result {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Detail
    )

    $status = if ($Passed) { "PASS" } else { "FAIL" }
    Write-Host ("[{0}] {1} - {2}" -f $status, $Name, $Detail)
    if (-not $Passed) {
        $script:failures.Add(("{0}: {1}" -f $Name, $Detail))
    }
}

function Test-FileContains {
    param(
        [string]$Path,
        [string]$Pattern
    )

    if (-not (Test-Path -LiteralPath $Path)) { return $false }
    $text = Get-Content -LiteralPath $Path -Raw
    return ($text -match $Pattern)
}

function Test-AllFilesContain {
    param(
        [string[]]$Paths,
        [string]$Pattern
    )

    foreach ($path in $Paths) {
        if (-not (Test-FileContains $path $Pattern)) { return $false }
    }

    return $true
}

function Get-C5DocText {
    $doc = Get-ChildItem -LiteralPath $projectRoot -Filter "*ESP32-C5.pdf" -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match "NM-CYD-C5" } |
        Select-Object -First 1

    if ($null -eq $doc) { return "" }

    $pdftotext = Get-Command pdftotext -ErrorAction SilentlyContinue
    if ($null -eq $pdftotext) { return "" }

    $tmp = Join-Path $env:TEMP ("nm-cyd-c5-doc-{0}.txt" -f ([guid]::NewGuid().ToString("N")))
    try {
        & $pdftotext.Source -layout $doc.FullName $tmp | Out-Null
        if (Test-Path -LiteralPath $tmp) {
            return (Get-Content -LiteralPath $tmp -Raw)
        }
    }
    finally {
        Remove-Item -LiteralPath $tmp -ErrorAction SilentlyContinue
    }

    return ""
}

function Get-PioEnvBlock {
    param(
        [string]$Path,
        [string]$EnvName
    )

    $lines = Get-Content -LiteralPath $Path
    $inside = $false
    $block = New-Object System.Collections.Generic.List[string]

    foreach ($line in $lines) {
        if ($line -match "^\s*\[env:") {
            if ($inside) { break }
            $inside = ($line -match "^\s*\[env:$([regex]::Escape($EnvName))\]\s*$")
        }

        if ($inside) {
            $block.Add($line)
        }
    }

    return $block
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
    Add-Result "PlatformIO" (Test-Path -LiteralPath $Pio) $Pio
    Add-Result "C5 board file" (Test-Path -LiteralPath "boards\nm_cyd_c5.json") "boards\nm_cyd_c5.json"
    $c5Doc = Get-ChildItem -LiteralPath $projectRoot -Filter "*ESP32-C5.pdf" -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match "NM-CYD-C5" } |
        Select-Object -First 1
    $c5DocText = Get-C5DocText
    Add-Result "C5 board PDF present" ($null -ne $c5Doc -and $c5Doc.Length -gt 0) ($(if ($c5Doc) { $c5Doc.Name } else { "missing NM-CYD-C5 ESP32-C5 board PDF" }))
    Add-Result "C5 board PDF specs" (($c5DocText -match "ESP32-C5-WROOM-1") -and ($c5DocText -match "16MB\s+Flash") -and ($c5DocText -match "8MB\s+PSRAM") -and ($c5DocText -match "Dual-band\s+Wi-?Fi\s+6") -and ($c5DocText -match "BLE\s+5\.3") -and ($c5DocText -match "ST7789") -and ($c5DocText -match "240\*320") -and ($c5DocText -match "Resistive\s+Touch")) "local NM-CYD-C5 docs prove ESP32-C5, dual-band WiFi 6/BLE 5.3, 16MB flash, 8MB PSRAM, ST7789 touch target"
    Add-Result "C5 flash monitor helper" (Test-Path -LiteralPath "scripts\flash_monitor_nm_cyd_c5.ps1") "waits for C5 port, flashes, optional monitor"
    Add-Result "C5 no-reset app flash" ((Test-FileContains "scripts\upload_nm_cyd_c5.ps1" "--after\s+no-reset") -and (Test-FileContains "scripts\upload_nm_cyd_c5.ps1" "0x10000") -and (Test-FileContains "scripts\flash_monitor_nm_cyd_c5.ps1" "--after\s+no-reset")) "helpers match the bench flash path and leave C5 ready for manual unplug/replug"
    Add-Result "C5 timed runtime capture" ((Test-FileContains "scripts\flash_monitor_nm_cyd_c5.ps1" "CaptureSeconds") -and (Test-FileContains "scripts\flash_monitor_nm_cyd_c5.ps1" "nm-cyd-c5-runtime") -and (Test-FileContains "scripts\flash_monitor_nm_cyd_c5.ps1" "\[C5-WIFI\]")) "flash helper can save runtime report log"
    Add-Result "C5 env" (Test-FileContains "platformio.ini" "\[env:nm-cyd-c5\]") "platformio.ini has nm-cyd-c5"
    Add-Result "C5 board binding" (Test-FileContains "platformio.ini" "board\s*=\s*nm_cyd_c5") "env uses nm_cyd_c5"
    Add-Result "C5 platform pin" ((Test-FileContains "platformio.ini" "55\.03\.38/platform-espressif32\.zip") -and (Test-FileContains "platformio.ini" "Arduino core 3\.3\.8")) "pioarduino 55.03.38 with Arduino ESP32 3.3.8"
    Add-Result "C5 compile flag" (Test-FileContains "platformio.ini" "ESP32C5_NM_CYD=1") "ESP32C5_NM_CYD defined"
    $c5EnvBlock = Get-PioEnvBlock "platformio.ini" "nm-cyd-c5"
    $c5HardcodedPort = $c5EnvBlock | Where-Object { $_ -match "^\s*upload_port\s*=" }
    Add-Result "C5 no hardcoded COM" ($null -eq $c5HardcodedPort) "upload script auto-detects port"
    Add-Result "C5 PSRAM memory profile" ((Test-FileContains "platformio.ini" "board_build\.flash_mode\s*=\s*dio") -and (Test-FileContains "platformio.ini" "board_build\.arduino\.memory_type\s*=\s*dio_qspi") -and (Test-FileContains "platformio.ini" "-DBOARD_HAS_PSRAM")) "C5 build uses DIO flash plus QSPI PSRAM profile"
    Add-Result "C5 calibration force opt-in" (Test-FileContains "WontHound-CYD.ino" "C5_FORCE_TOUCH_CALIBRATION_ON_BOOT\s+0") "normal C5 firmware only calibrates when no saved touch calibration exists"
    Add-Result "C5 base menu hides NRF paths" ((Test-FileContains "WontHound-CYD.ino" "#if\s+CYD_HAS_NRF24[\s\S]*main_menu_page0\[\]\s*=\s*\{0,\s*1,\s*2,\s*5,\s*7,\s*MAIN_MENU_NEXT_PAGE\}[\s\S]*#else[\s\S]*main_menu_page0\[\]\s*=\s*\{0,\s*1,\s*5,\s*7,\s*MAIN_MENU_NEXT_PAGE\}") -and (Test-FileContains "cyd_config.h" "#elif defined\(ESP32C5_NM_CYD\)[\s\S]*CYD_HAS_NRF24\s+0")) "C5 base profile does not advertise the external NRF24 2.4GHz menu"
    Add-Result "C5 jam menu hardware-gated" (Test-FileContains "WontHound-CYD.ino" "#if\s+CYD_HAS_CC1101\s*\|\|\s*CYD_HAS_NRF24[\s\S]*jamdetect_NUM_SUBMENU_ITEMS\s*=\s*5[\s\S]*#else[\s\S]*jamdetect_NUM_SUBMENU_ITEMS\s*=\s*2[\s\S]*WiFi Guardian[\s\S]*Back to Main Menu") "C5 base profile exposes WiFi Guardian without dead CC1101/NRF watchdog entries"
    Add-Result "C5 jam handler hardware-gated" ((Test-FileContains "WontHound-CYD.ino" "const int backIndex[\s\S]*#if\s+CYD_HAS_CC1101\s*\|\|\s*CYD_HAS_NRF24[\s\S]*4;[\s\S]*#else[\s\S]*1;[\s\S]*if\s*\(current_submenu_index\s*==\s*backIndex\)") -and (Test-FileContains "WontHound-CYD.ino" "#if\s+CYD_HAS_CC1101\s*\|\|\s*CYD_HAS_NRF24[\s\S]*case 1:\s*// SubGHz Sentinel[\s\S]*case 3:\s*// Full Spectrum[\s\S]*#endif")) "C5 Jam Detect Back item cannot fall through into hidden CC1101/NRF modules"

    Add-Result "US 2.4 GHz channel count" (Test-FileContains "wifi_band_utils.h" "WONTHOUND_WIFI_2G_CHANNEL_COUNT\s+11") "US 2.4 GHz sweep is channels 1-11"
    Add-Result "5 GHz channel table" ((Test-FileContains "wifi_band_utils.h" "WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT\s+25") -and (Test-FileContains "wifi_band_utils.cpp" "36,\s*40,\s*44,\s*48,\s*[\s\S]*52,\s*56,\s*60,\s*64[\s\S]*100,\s*104,\s*108,\s*112[\s\S]*149,\s*153,\s*157,\s*161,\s*165")) "active 5 GHz scan/hop channels include lower, DFS, and upper bands"
    Add-Result "Dual-band fast sweep mode" ((Test-FileContains "wifi_band_utils.h" "WONTHOUND_WIFI_DUAL_BAND_FAST_SWEEP\s+1") -and (Test-FileContains "wifi_band_utils.cpp" "dual-band-fast-sweep") -and (Test-FileContains "wifi_band_utils.cpp" "single_radio=yes")) "C5 reports fast 2.4/5 GHz switching instead of pretending simultaneous radios"
    Add-Result "Interleaved 2.4/5 GHz channel order" (Test-FileContains "wifi_band_utils.cpp" "1,\s*36,\s*6,\s*40,\s*11,\s*44,\s*2,\s*48,\s*3,\s*149") "channel hop order reaches 5 GHz immediately"
    Add-Result "US 5 GHz country mask" ((Test-FileContains "wifi_band_utils.cpp" "WIFI_COUNTRY_POLICY_MANUAL") -and (Test-FileContains "wifi_band_utils.cpp" "WIFI_CHANNEL_36") -and (Test-FileContains "wifi_band_utils.cpp" "WIFI_CHANNEL_52") -and (Test-FileContains "wifi_band_utils.cpp" "WIFI_CHANNEL_144") -and (Test-FileContains "wifi_band_utils.cpp" "WIFI_CHANNEL_165")) "C5 config applies broad US manual 5 GHz channel policy"
    Add-Result "Country-safe channel hops" (Test-FileContains "wifi_band_utils.cpp" "void wh_wifi_prepare_channel\(uint8_t channel\)[\s\S]*wh_wifi_configure_country\(\);[\s\S]*esp_wifi_set_band_mode") "all direct C5 channel hops apply country policy"
    Add-Result "WiFi 6 protocol" (Test-FileContains "wifi_band_utils.cpp" "WIFI_PROTOCOL_11AX") "11ax enabled for C5"
    Add-Result "WiFi 6-safe bandwidths" ((Test-FileContains "wifi_band_utils.cpp" "\.ghz_2g\s*=\s*WIFI_BW20") -and (Test-FileContains "wifi_band_utils.cpp" "\.ghz_5g\s*=\s*WIFI_BW20") -and (-not (Test-FileContains "wifi_band_utils.cpp" "\.ghz_5g\s*=\s*WIFI_BW40"))) "C5 stays on 20 MHz for 11ax-friendly operation"
    Add-Result "C5 runtime WiFi report" ((Test-FileContains "wifi_band_utils.cpp" "wh_wifi_print_c5_runtime_report") -and (Test-FileContains "WontHound-CYD.ino" "wh_wifi_print_c5_runtime_report\(\)")) "boot serial report covers C5 band/country/protocol state"
    Add-Result "C5 runtime PSRAM report" (Test-FileContains "WontHound-CYD.ino" "#if defined\(ESP32S3_ES3C28P\) \|\| defined\(ESP32C5_NM_CYD\)[\s\S]*\[INIT\] PSRAM: %s, free=%u total=%u") "boot serial report covers C5 PSRAM presence and size"
    Add-Result "C5 PSRAM cacheline self-test" ((Test-FileContains "WontHound-CYD.ino" '#include\s+<esp_heap_caps\.h>') -and (Test-FileContains "WontHound-CYD.ino" "heap_caps_aligned_calloc[\s\S]*MALLOC_CAP_SPIRAM[\s\S]*PSRAM cacheline self-test") -and (Test-FileContains "WontHound-CYD.ino" "psramProbe\[0\]\s*==\s*0xA5[\s\S]*psramProbe\[psramLineSize - 1\]\s*==\s*0x5A")) "boot serial report verifies C5 PSRAM aligned allocation and read/write after early IDF warning"
    Add-Result "C5 touch consume respected" ((Test-FileContains "touch_buttons.cpp" "_touchFired\s*&&\s*!peekOnly") -and (Test-FileContains "touch_buttons.cpp" "if\s*\(getTouchPoint\(&touchX,\s*&touchY\)\)")) "feature icon taps can block generic Back zones"
    Add-Result "C5 top-left Back zone" (Test-FileContains "cyd_config.h" "defined\(ESP32S3_ES3C28P\)\s*\|\|\s*defined\(ESP32C5_NM_CYD\)[\s\S]*TOUCH_BTN_BACK_X1\s+0[\s\S]*TOUCH_BTN_BACK_X2\s+64") "C5 generic Back zone matches visible top-left icon"
    $channelFiles = @(
        "eapol_capture.cpp",
        "iot_recon.cpp",
        "jam_detect.cpp",
        "karma_attack.cpp",
        "nrf24_attacks.cpp",
        "wardriving_screen.cpp"
    )
    Add-Result "WiFi channel files include band helper" (Test-AllFilesContain $channelFiles '#include\s+"wifi_band_utils\.h"') "channel hop and scan files compile with C5 band utilities"
    Add-Result "WiFi attacks include band helper" ((Test-FileContains "wifi_attacks.h" '#include\s+"wifi_band_utils\.h"') -and (Test-FileContains "wifi_attacks.cpp" '#include\s+"wifi_attacks\.h"')) "wifi_attacks.cpp gets C5 channel wrapper through wifi_attacks.h"
    Add-Result "C5 channel wrapper active" ((Test-FileContains "wifi_band_utils.h" "#define\s+esp_wifi_set_channel") -and (Test-FileContains "wifi_band_utils.cpp" "WONTHOUND_WIFI_BAND_UTILS_IMPL")) "direct esp_wifi_set_channel calls route through wh_esp_wifi_set_channel on C5"
    Add-Result "Scan paths prep dual-band" ((Test-FileContains "eapol_capture.cpp" "wh_wifi_prepare_scan_dual_band\(\)[\s\S]*WiFi\.scanNetworks") -and (Test-FileContains "iot_recon.cpp" "wh_wifi_prepare_scan_dual_band\(\)[\s\S]*WiFi\.scanNetworks") -and (Test-FileContains "nrf24_attacks.cpp" "wh_wifi_prepare_scan_dual_band\(\)[\s\S]*WiFi\.scanNetworks") -and (Test-FileContains "wifi_attacks.cpp" "wh_wifi_prepare_scan_dual_band\(\)[\s\S]*WiFi\.scanNetworks") -and (Test-FileContains "wardriving_screen.cpp" "wh_wifi_prepare_scan_dual_band\(\)[\s\S]*WiFi\.scanNetworks")) "broad WiFi scans configure AUTO 2.4/5 GHz before scanning"
    Add-Result "Wardriving per-channel prep" (Test-FileContains "wardriving_screen.cpp" "wh_wifi_prepare_channel\(ch\);[\s\S]*WiFi\.scanNetworks\(false,\s*true,\s*false,\s*200,\s*ch\)") "wardriving switches 2.4/5 GHz mode before channel-specific scan"
    Add-Result "Hop loops use C5 channel table" ((Test-FileContains "jam_detect.cpp" "wh_wifi_channel_count\(\)") -and (Test-FileContains "karma_attack.cpp" "wh_wifi_channel_at") -and (Test-FileContains "wifi_attacks.cpp" "#define\s+MAX_CH\s+wh_wifi_channel_count\(\)") -and (Test-FileContains "wifi_attacks.cpp" "#define\s+MAX_CHANNEL\s+wh_wifi_channel_count\(\)")) "sniffers and attacks sweep the shared 2.4/5 GHz channel list"
    Add-Result "AP band prep helper" (Test-FileContains "wifi_band_utils.h" "wh_wifi_prepare_ap_channel") "AP/raw-frame paths have band helper"
    Add-Result "AP raw paths patched" ((Test-FileContains "wifi_attacks.cpp" "wh_wifi_prepare_ap_channel\(selectedChannel\)") -and (Test-FileContains "wifi_attacks.cpp" "wh_wifi_prepare_ap_channel\(targetChannel\)") -and (Test-FileContains "wifi_attacks.cpp" "wh_wifi_prepare_ap_channel\(ch\)")) "deauth/auth flood/portal prepare target band"
    Add-Result "Deauther explicit dual-band scan" ((Test-FileContains "wifi_band_utils.cpp" "wh_wifi_scan_dual_band_records[\s\S]*channel_bitmap\.ghz_2_channels[\s\S]*channel_bitmap\.ghz_5_channels") -and (Test-FileContains "wifi_attacks.cpp" "C5 dual-band ESP-IDF scan") -and (Test-FileContains "wifi_attacks.cpp" "maxScanNetworks\s*=\s*128")) "Deauther scans visible 2.4/5 GHz channel records instead of relying on narrow Arduino scan output"
    Add-Result "C5 target lists use explicit scan" ((Test-FileContains "eapol_capture.cpp" "C5 dual-band ESP-IDF scan") -and (Test-FileContains "iot_recon.cpp" "C5 dual-band ESP-IDF scan") -and (Test-FileContains "wifi_attacks.cpp" "\[AUTHFLOOD\] C5 dual-band ESP-IDF scan") -and (Test-FileContains "wifi_attacks.cpp" "\[WIFISCAN\] C5 dual-band ESP-IDF scan") -and (Test-FileContains "wifi_attacks.cpp" "\[PORTAL\] C5 dual-band ESP-IDF scan")) "EAPOL, IoT Recon, AuthFlood, WiFi Scan, and Portal use explicit 2.4/5 GHz records on C5"
    Add-Result "WiFi scan handoff uses C5 records" ((Test-FileContains "wifi_attacks.cpp" "static wifi_ap_record_t wsApList") -and (Test-FileContains "wifi_attacks.cpp" "selectedChannel = wsChannel\(realIdx\)") -and (Test-FileContains "wifi_attacks.cpp" "wardrivingLogNetwork\([\s\S]*wsBSSID\(i\)[\s\S]*wsChannel\(i\)")) "WiFi Scan sorting/filtering/attack handoff/wardriving reads C5 dual-band scan records"
    Add-Result "Deauther C5 handoff shows picker" ((Test-FileContains "wifi_attacks.cpp" "targetReviewPending") -and (Test-FileContains "wifi_attacks.cpp" "showPresetInPicker") -and (Test-FileContains "wifi_attacks.cpp" "findApByBSSID") -and (Test-FileContains "wifi_attacks.cpp" "DUAL AP:")) "C5 Deauther handoff keeps 2.4/5 GHz channel list visible before attack"
    Add-Result "Deauther C5 blocks auto-attack handoff" ((Test-FileContains "wifi_attacks.cpp" "#if WONTHOUND_HAS_WIFI_5G[\s\S]*targetPreset = false;[\s\S]*targetReviewPending = true;[\s\S]*#endif[\s\S]*const bool showPresetInPicker") -and (Test-FileContains "wifi_attacks.cpp" "#if !WONTHOUND_HAS_WIFI_5G[\s\S]*if \(targetPreset\)[\s\S]*drawAttackScreen\(\);[\s\S]*#endif[\s\S]*scanNetworks\(\);")) "C5 Deauther cannot skip directly to TX from WifiScan handoff"
    Add-Result "Deauther C5 fresh-input gate" ((Test-FileContains "wifi_attacks.cpp" "lockInputAfterScreenChange[\s\S]*clearButtonEvents\(\);[\s\S]*consumeTouch\(\);") -and (Test-FileContains "wifi_attacks.cpp" "millis\(\) - inputUnlockAt[\s\S]*clearButtonEvents\(\);[\s\S]*return;") -and (Test-FileContains "wifi_attacks.cpp" "void startAttack\(\)[\s\S]*selectedApIndex < 0[\s\S]*drawScanScreen\(\);")) "C5 Deauther cannot inherit stale menu/touch input or start TX without a selected AP"
    Add-Result "Deauther passive target selection" ((Test-FileContains "wifi_attacks.cpp" "Keep AP selection passive; raw deauth radio mode starts only after Start\.") -and (Test-FileContains "wifi_attacks.cpp" "lockStartAfterTargetSelect\(\);") -and (Test-FileContains "wifi_attacks.cpp" "else if \(\(int32_t\)\(millis\(\) - startUnlockAt\) >= 0\) \{\s*startAttack\(\);") -and (Test-FileContains "wifi_attacks.cpp" "void startAttack\(\)[\s\S]*if \(!inAttackMode\) initAttackMode\(\);")) "selecting an AP only opens the stopped target screen; raw TX mode begins on explicit Start after the target-selection debounce"
    Add-Result "Deauther dual-band AP list visibility" ((Test-FileContains "wifi_attacks.cpp" "interleaveDualBandApList") -and (Test-FileContains "wifi_attacks.cpp" "first-page=interleaved") -and (Test-FileContains "wifi_attacks.cpp" 'tft\.print\(" 2G:"\)') -and (Test-FileContains "wifi_attacks.cpp" 'tft\.print\(" 5G:"\)')) "C5 deauther interleaves 2.4/5 GHz APs and shows band counts instead of burying 5 GHz rows"
    Add-Result "Deauther raw touch before generic zones" ((Test-FileContains "wifi_attacks.cpp" "Module-specific touch handling runs before generic virtual zones[\s\S]*peekTouchPoint\(&tx, &ty\)[\s\S]*Touch handling for attack screen[\s\S]*touchButtonsUpdate\(\);") -and (Test-FileContains "wifi_attacks.cpp" 'C5 2\.8" virtual zones overlap rows and bottom tabs')) "C5 Deauther row taps and Start/Back taps are handled before virtual UP/SELECT/DOWN zones"
    Add-Result "Deauther owns feature input loop" (-not (Test-FileContains "WontHound-CYD.ino" "Deauther::loop\(\);\s*touchButtonsUpdate\(\);")) "outer menu loop does not re-read stale touch/button input after Deauther handles it"
    Add-Result "WiFi feature wrappers own input" ((-not (Test-FileContains "WontHound-CYD.ino" "PacketMonitor::loop\(\);\s*touchButtonsUpdate\(\);")) -and (-not (Test-FileContains "WontHound-CYD.ino" "BeaconSpammer::loop\(\);\s*touchButtonsUpdate\(\);")) -and (-not (Test-FileContains "WontHound-CYD.ino" "DeauthDetect::loop\(\);\s*touchButtonsUpdate\(\);")) -and (-not (Test-FileContains "WontHound-CYD.ino" "CaptivePortal::loop\(\);\s*touchButtonsUpdate\(\);")) -and (-not (Test-FileContains "WontHound-CYD.ino" "AuthFlood::loop\(\);\s*touchButtonsUpdate\(\);")) -and (Test-FileContains "WontHound-CYD.ino" "BeaconSpammer::loop\(\);\s*if \(BeaconSpammer::isExitRequested\(\)\)")) "Packet Monitor, Beacon, Probe, Portal, and AuthFlood wrappers do not double-read touch input after module handling"
    Add-Result "AuthFlood raw touch before generic zones" ((Test-FileContains "wifi_attacks.cpp" "Touch to select network before the generic virtual-button layer can consume it\.[\s\S]*Generic buttons are only sampled after AuthFlood's own touch UI\.") -and (-not (Test-FileContains "wifi_attacks.cpp" "namespace AuthFlood[\s\S]*void loop\(\)\s*\{\s*if \(!initialized\) return;\s*touchButtonsUpdate\(\);"))) "AuthFlood target rows are handled before virtual touch zones"
    Add-Result "Probe sniffer raw touch before generic zones" (-not (Test-FileContains "wifi_attacks.cpp" "namespace DeauthDetect[\s\S]*void loop\(\)\s*\{\s*if \(!initialized\) return;\s*touchButtonsUpdate\(\);")) "Probe/DeauthDetect does not pre-consume icon/list touches through generic zones"
    Add-Result "Beacon frames 5 GHz-aware" ((Test-FileContains "wifi_attacks.cpp" "buildBeaconFrame") -and (Test-FileContains "wifi_attacks.cpp" "rates5g") -and (Test-FileContains "wifi_attacks.cpp" "DS Parameter Set is 2\.4 GHz only")) "beacon spammer changes IEs by band"
    Add-Result "WiFi attack UI shows band" ((Test-FileContains "wifi_attacks.cpp" "wh_wifi_band_label\(currentChannel\)") -and (Test-FileContains "wifi_attacks.cpp" "wh_wifi_band_label\(bsChannel\)") -and (Test-FileContains "wifi_attacks.cpp" "wh_wifi_band_label\(apList\[i\]\.primary\)") -and (Test-FileContains "wifi_attacks.cpp" "wh_wifi_band_label\(selectedChannel\)")) "Packet Monitor, Beacon Spam, and Deauther show 2G/5G channel state"
    Add-Result "Probe and station UI shows band" ((Test-FileContains "wifi_attacks.cpp" 'PRB%d/%s') -and (Test-FileContains "wifi_attacks.cpp" 'CH:%d/%s", currentChannel') -and (Test-FileContains "wifi_attacks.cpp" 'wh_wifi_band_label\(s->apChannel\)') -and (Test-FileContains "wifi_attacks.cpp" 'selectedChannels\[selectedCount\]\s*=\s*wh_normalize_wifi_channel')) "probe sniffer and station scanner preserve visible 2G/5G channel state"
    Add-Result "Guardian and Karma show band" ((Test-FileContains "jam_detect.cpp" 'Deauth %lu/s ch%d/%s') -and (Test-FileContains "jam_detect.cpp" 'Ch:%d/%s RSSI') -and (Test-FileContains "jam_detect.cpp" 'WiFi ch%d/%s') -and (Test-FileContains "karma_attack.cpp" 'CH:%d/%s')) "guardian/full-spectrum/karma screens expose current 2.4/5 GHz hop"
    Add-Result "AuthFlood and portal show band" ((Test-FileContains "wifi_attacks.cpp" 'wh_wifi_band_label\(targetAp\.primary\)') -and (Test-FileContains "wifi_attacks.cpp" 'CH:%d/%s BSSID') -and (Test-FileContains "wifi_attacks.cpp" 'CH%d/%s\\n"') -and (Test-FileContains "wifi_attacks.cpp" 'terminalPrint\("\[\*\] CH: " \+ String\(ch\) \+ "/"')) "AuthFlood, captured PSKs, and captive portal active screen/logs include 2G/5G band state"
    Add-Result "EAPOL capture shows band" ((Test-FileContains "eapol_capture.cpp" 'wh_wifi_band_label\(apList\[selectedAP\]\.channel\)') -and (Test-FileContains "eapol_capture.cpp" 'CH:%d/%s\s+%d frames/burst') -and (Test-FileContains "eapol_capture.cpp" 'ch=%d/%s')) "EAPOL target, deauth burst, and serial state include 2G/5G band"
    Add-Result "Beacon icon handling before generic touch" (Test-FileContains "wifi_attacks.cpp" "Run before touchButtonsUpdate\(\).*top-right[\s\S]*peekTouchPoint\(&tx,\s*&ty\)[\s\S]*touchButtonsUpdate\(\);") "C5 Beacon Spam start/nuke/channel icons do not fall through to Back"
    Add-Result "EAPOL AP channel patched" (Test-FileContains "eapol_capture.cpp" "wh_wifi_prepare_ap_channel\(apList\[selectedAP\]\.channel\)") "EAPOL hidden AP prepares target band"
    Add-Result "BLE C5 branch" (Test-FileContains "bluetooth_attacks.cpp" "#if defined\(ESP32C5_NM_CYD\)") "C5 Bluetooth implementation is compiled"
    Add-Result "BLE no C5 stub marker" (-not ((Test-FileContains "bluetooth_attacks.cpp" "C5_BLE_STUB") -or (Test-FileContains "bluetooth_attacks.cpp" "c5BleLegacyUnavailable"))) "C5 BLE modules are real implementations"
    Add-Result "BLE jammer hardware gate" ((Test-FileContains "bluetooth_attacks.cpp" '#if !CYD_HAS_NRF24[\s\S]*External NRF24 required') -and (Test-FileContains "bluetooth_attacks.cpp" 'NRF24 DISABLED IN C5 BASE')) "C5 base profile does not pretend onboard BLE can do external NRF24 carrier jamming"
    Add-Result "BLE menu reaches C5 modules" ((Test-FileContains "WontHound-CYD.ino" 'bluetooth_NUM_SUBMENU_ITEMS\s*=\s*7[\s\S]*"BLE Jammer"[\s\S]*"BLE Spoofer"[\s\S]*"BLE Beacon"[\s\S]*"BLE Predator"[\s\S]*"WhisperPair"[\s\S]*"Lunatic Fringe"[\s\S]*"Back to Main Menu"') -and (Test-FileContains "WontHound-CYD.ino" 'case 0:\s*// BLE Jammer[\s\S]*BleJammer::setup\(\)[\s\S]*case 1:\s*// BLE Spoofer[\s\S]*BleSpoofer::setup\(\)[\s\S]*case 2:\s*// BLE Beacon[\s\S]*BleBeacon::setup\(\)[\s\S]*case 3:\s*// BLE Predator[\s\S]*BlePredator::setup\(\)[\s\S]*case 4:\s*// WhisperPair[\s\S]*WhisperPair::setup\(\)[\s\S]*case 5:\s*// Lunatic Fringe[\s\S]*handleLunaticFringeHubTouch\(\)')) "Bluetooth menu exposes C5 BLE jammer, advertiser, predator, WhisperPair, and tracker hub routes"
    Add-Result "BLE tracker hub reaches C5 modules" ((Test-FileContains "WontHound-CYD.ino" 'lunafringe_NUM_SUBMENU_ITEMS\s*=\s*5[\s\S]*"Tracker Scan"[\s\S]*"AirTag Detect"[\s\S]*"Phantom Flood"[\s\S]*"AirTag Replay"[\s\S]*"Back"') -and (Test-FileContains "WontHound-CYD.ino" 'case 0:\s*// Tracker Scan[\s\S]*LunaticFringe::setup\(\)[\s\S]*case 1:\s*// AirTag Detect[\s\S]*AirTagDetect::setup\(\)[\s\S]*case 2:\s*// Phantom Flood[\s\S]*PhantomFlood::setup\(\)[\s\S]*case 3:\s*// AirTag Replay[\s\S]*AirTagReplay::setup\(\)')) "Lunatic Fringe hub exposes tracker scan, AirTag detect, Phantom flood, and AirTag replay"
    Add-Result "BLE SIGINT Flock route" ((Test-FileContains "WontHound-CYD.ino" 'sigint_NUM_SUBMENU_ITEMS\s*=\s*8[\s\S]*"Flock You"') -and (Test-FileContains "WontHound-CYD.ino" 'case 6:\s*// Flock You[\s\S]*FlockYou::setup\(\)[\s\S]*FlockYou::cleanup\(\)')) "Flock You BLE detector remains reachable from SIGINT"
    Add-Result "BLE C5 namespaces map to helpers" ((Test-FileContains "bluetooth_attacks.cpp" 'namespace BleSpoofer \{[\s\S]*void setup\(\) \{ c5SetupAdvertiser\("BLE SPOOF", C5_ADV_SPOOFER\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace BleBeacon \{[\s\S]*void setup\(\) \{ c5SetupAdvertiser\("BLE BEACON", C5_ADV_BEACON\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace PhantomFlood \{[\s\S]*void setup\(\) \{ c5SetupAdvertiser\("PHANTOM", C5_ADV_PHANTOM\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace BleJammer \{[\s\S]*void setup\(\) \{ c5SetupJammer\(\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace AirTagReplay \{[\s\S]*void setup\(\) \{ c5SetupReplay\("AIRTAG REPLAY", true\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace BlePredator \{[\s\S]*void setup\(\) \{ c5SetupReplay\("BLE PREDATOR", false\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace WhisperPair \{[\s\S]*void setup\(\) \{ c5SetupWhisper\(\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace AirTagDetect \{[\s\S]*void setup\(\) \{ c5SetupScanner\("AIRTAG DETECT", C5_BLE_VIEW_TRACKERS\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace LunaticFringe \{[\s\S]*void setup\(\) \{ c5SetupScanner\("TRACKER SCAN", C5_BLE_VIEW_TRACKERS\); \}') -and (Test-FileContains "bluetooth_attacks.cpp" 'namespace FlockYou \{[\s\S]*void setup\(\) \{ c5SetupScanner\("FLOCK DETECT", C5_BLE_VIEW_FLOCK\); \}')) "C5 BLE public namespaces dispatch into C5 scanner/advertiser/replay/whisper/jammer helpers"
    Add-Result "BLE C5 radio lifecycle" ((Test-FileContains "bluetooth_attacks.cpp" 'c5SetupScanner[\s\S]*WiFi\.mode\(WIFI_OFF\)[\s\S]*c5CleanupScanner[\s\S]*BLEDevice::deinit\(false\)[\s\S]*WiFi\.mode\(WIFI_STA\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5SetupAdvertiser[\s\S]*WiFi\.mode\(WIFI_OFF\)[\s\S]*c5CleanupAdvertiser[\s\S]*BLEDevice::deinit\(false\)[\s\S]*WiFi\.mode\(WIFI_STA\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5SetupReplay[\s\S]*WiFi\.mode\(WIFI_OFF\)[\s\S]*c5CleanupReplay[\s\S]*BLEDevice::deinit\(false\)[\s\S]*WiFi\.mode\(WIFI_STA\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5SetupWhisper[\s\S]*WiFi\.mode\(WIFI_OFF\)[\s\S]*c5CleanupWhisper[\s\S]*BLEDevice::deinit\(false\)[\s\S]*WiFi\.mode\(WIFI_STA\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5SetupJammer[\s\S]*WiFi\.mode\(WIFI_OFF\)[\s\S]*BLEDevice::deinit\(false\)[\s\S]*c5CleanupJammer[\s\S]*WiFi\.mode\(WIFI_STA\)')) "C5 BLE modes stop WiFi before BLE/NRF work and restore STA mode on cleanup"
    Add-Result "BLE C5 stack init paths" ((Test-FileContains "bluetooth_attacks.cpp" 'c5StartScan[\s\S]*BLEDevice::init\("WontHound-C5"\)[\s\S]*BLEDevice::getScan\(\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5TransmitAdvertisement[\s\S]*BLEDevice::init\("WontHound-C5"\)[\s\S]*BLEDevice::getAdvertising\(\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5RunReplayScan[\s\S]*BLEDevice::init\("WontHound-C5"\)[\s\S]*BLEDevice::getScan\(\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5TransmitReplay[\s\S]*BLEDevice::init\("WontHound-C5"\)[\s\S]*BLEDevice::getAdvertising\(\)') -and (Test-FileContains "bluetooth_attacks.cpp" 'c5RunWhisperScan[\s\S]*BLEDevice::init\("WontHound-C5"\)[\s\S]*BLEDevice::getScan\(\)')) "C5 BLE scan, advertiser, replay, and WhisperPair paths explicitly init the BLE stack"

    $firmware = Get-Item -LiteralPath ".pio\build\nm-cyd-c5\firmware.bin" -ErrorAction SilentlyContinue
    $factory = Get-Item -LiteralPath ".pio\build\nm-cyd-c5\firmware.factory.bin" -ErrorAction SilentlyContinue
    Add-Result "firmware.bin" ($null -ne $firmware -and $firmware.Length -gt 0) ($(if ($firmware) { "$($firmware.Length) bytes, $($firmware.LastWriteTime)" } else { "missing" }))
    Add-Result "firmware.factory.bin" ($null -ne $factory -and $factory.Length -gt 0) ($(if ($factory) { "$($factory.Length) bytes, $($factory.LastWriteTime)" } else { "missing" }))

    $port = Get-C5UploadPort
    Add-Result "C5 upload port visible" (-not [string]::IsNullOrWhiteSpace($port)) ($(if ($port) { $port } else { "no Espressif/CP210/CH340/FTDI/JTAG COM port exposed by Windows" }))

    if ($failures.Count -gt 0) {
        Write-Host ""
        Write-Host "C5 audit found blocking items:"
        foreach ($failure in $failures) { Write-Host (" - {0}" -f $failure) }
        exit 1
    }

    Write-Host ""
    Write-Host "C5 audit passed."
    exit 0
}
finally {
    Pop-Location
}
