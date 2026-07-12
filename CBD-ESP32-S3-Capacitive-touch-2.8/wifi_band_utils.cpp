#define WONTHOUND_WIFI_BAND_UTILS_IMPL
#include "wifi_band_utils.h"
#include "radio_power_utils.h"

const uint8_t WH_WIFI_2G_CHANNELS[WONTHOUND_WIFI_2G_CHANNEL_COUNT] = {
    1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10
};

#if WONTHOUND_HAS_WIFI_5G
const uint8_t WH_WIFI_5G_ACTIVE_CHANNELS[WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT] = {
    36, 40, 44, 48,
    52, 56, 60, 64,
    100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
    149, 153, 157, 161, 165
};

static const uint8_t WH_WIFI_FAST_SWEEP_CHANNELS[WONTHOUND_WIFI_2G_CHANNEL_COUNT + WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT] = {
    1, 36, 6, 40, 11, 44, 2, 48, 3, 149, 4, 153, 5, 157, 7, 161, 8, 165,
    9, 52, 10, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144
};
#endif

bool wh_wifi_is_5g_channel(uint8_t channel) {
    return channel >= 32;
}

const char* wh_wifi_band_label(uint8_t channel) {
    return wh_wifi_is_5g_channel(channel) ? "5G" : "2G";
}

const char* wh_wifi_dual_band_mode_label() {
#if WONTHOUND_HAS_WIFI_5G
    return WONTHOUND_WIFI_DUAL_BAND_FAST_SWEEP ? "dual-band-fast-sweep" : "single-band";
#else
    return "2g-only";
#endif
}

uint8_t wh_wifi_channel_count() {
#if WONTHOUND_HAS_WIFI_5G
    return WONTHOUND_WIFI_2G_CHANNEL_COUNT + WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT;
#else
    return WONTHOUND_WIFI_2G_CHANNEL_COUNT;
#endif
}

uint8_t wh_wifi_channel_at(uint8_t index) {
    const uint8_t count = wh_wifi_channel_count();
    if (count == 0) return 1;
    index %= count;

#if WONTHOUND_HAS_WIFI_5G
    return WH_WIFI_FAST_SWEEP_CHANNELS[index];
#else
    if (index < WONTHOUND_WIFI_2G_CHANNEL_COUNT) {
        return WH_WIFI_2G_CHANNELS[index];
    }

    return WH_WIFI_2G_CHANNELS[0];
#endif
}

uint8_t wh_normalize_wifi_channel(int channel) {
    for (uint8_t i = 0; i < wh_wifi_channel_count(); i++) {
        if (wh_wifi_channel_at(i) == channel) return (uint8_t)channel;
    }
    return WH_WIFI_2G_CHANNELS[0];
}

uint8_t wh_next_wifi_channel(uint8_t current, int direction) {
    const uint8_t count = wh_wifi_channel_count();
    int idx = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (wh_wifi_channel_at(i) == current) {
            idx = i;
            break;
        }
    }
    idx = (idx + direction) % count;
    if (idx < 0) idx += count;
    return wh_wifi_channel_at((uint8_t)idx);
}

uint8_t wh_random_wifi_channel() {
    return wh_wifi_channel_at((uint8_t)random(0, wh_wifi_channel_count()));
}

void wh_wifi_configure_country() {
    wifi_country_t desired = {};
    desired.cc[0] = 'U';
    desired.cc[1] = 'S';
    desired.cc[2] = ' ';
    desired.schan = 1;
    desired.nchan = 11;
    // wifi_country_t stores whole dBm. esp_wifi_set_max_tx_power() separately
    // uses quarter-dBm units, so its corresponding maximum request is 80.
    desired.max_tx_power = 20;
    desired.policy = WIFI_COUNTRY_POLICY_MANUAL;
#if CONFIG_SOC_WIFI_SUPPORT_5G
    desired.wifi_5g_channel_mask =
        WIFI_CHANNEL_36 | WIFI_CHANNEL_40 | WIFI_CHANNEL_44 | WIFI_CHANNEL_48 |
        WIFI_CHANNEL_52 | WIFI_CHANNEL_56 | WIFI_CHANNEL_60 | WIFI_CHANNEL_64 |
        WIFI_CHANNEL_100 | WIFI_CHANNEL_104 | WIFI_CHANNEL_108 | WIFI_CHANNEL_112 |
        WIFI_CHANNEL_116 | WIFI_CHANNEL_120 | WIFI_CHANNEL_124 | WIFI_CHANNEL_128 |
        WIFI_CHANNEL_132 | WIFI_CHANNEL_136 | WIFI_CHANNEL_140 | WIFI_CHANNEL_144 |
        WIFI_CHANNEL_149 | WIFI_CHANNEL_153 | WIFI_CHANNEL_157 | WIFI_CHANNEL_161 |
        WIFI_CHANNEL_165;
#endif

    wifi_country_t current = {};
    if (esp_wifi_get_country(&current) == ESP_OK) {
        bool same = current.cc[0] == desired.cc[0] &&
                    current.cc[1] == desired.cc[1] &&
                    current.schan == desired.schan &&
                    current.nchan == desired.nchan &&
                    current.max_tx_power == desired.max_tx_power &&
                    current.policy == desired.policy;
#if CONFIG_SOC_WIFI_SUPPORT_5G
        same = same && current.wifi_5g_channel_mask == desired.wifi_5g_channel_mask;
#endif
        if (same) return;
    }

    esp_err_t err = esp_wifi_set_country(&desired);
    if (err != ESP_OK) {
        Serial.printf("[S3-RF] country setup failed: 0x%x\n", err);
    }
}

void wh_wifi_configure_dual_band() {
#if WONTHOUND_HAS_WIFI_5G
    wh_wifi_configure_country();

    // ESP32-C5 AUTO selects either band for the single radio; it is not simultaneous dual-band.
    esp_wifi_set_band_mode(WIFI_BAND_MODE_AUTO);

    wifi_protocols_t protocols = {
        .ghz_2g = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_11AX,
        .ghz_5g = WIFI_PROTOCOL_11A | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_11AX
    };
    esp_wifi_set_protocols(WIFI_IF_STA, &protocols);
    esp_wifi_set_protocols(WIFI_IF_AP, &protocols);

    wifi_bandwidths_t bandwidths = {
        .ghz_2g = WIFI_BW20,
        .ghz_5g = WIFI_BW20
    };
    esp_wifi_set_bandwidths(WIFI_IF_STA, &bandwidths);
    esp_wifi_set_bandwidths(WIFI_IF_AP, &bandwidths);
#endif
}

void wh_wifi_prepare_scan_dual_band() {
#if WONTHOUND_HAS_WIFI_5G
    wh_wifi_configure_dual_band();
#endif
}

void wh_wifi_prepare_channel(uint8_t channel) {
#if WONTHOUND_HAS_WIFI_5G
    channel = wh_normalize_wifi_channel(channel);
    wh_wifi_configure_country();
    esp_wifi_set_band_mode(wh_wifi_is_5g_channel(channel) ? WIFI_BAND_MODE_5G_ONLY : WIFI_BAND_MODE_2G_ONLY);
#else
    (void)channel;
#endif
}

void wh_wifi_prepare_ap_channel(uint8_t channel) {
#if WONTHOUND_HAS_WIFI_5G
    wh_wifi_configure_dual_band();
    wh_wifi_prepare_channel(channel);
#else
    (void)channel;
#endif
}

bool wh_wifi_prepare_sta_scan() {
    WiFi.persistent(false);
    WiFi.scanDelete();

    // Raw monitor/attack modules may leave promiscuous callbacks registered.
    // Shut them down before returning ownership to Arduino's WiFi event layer.
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);

    bool offOk = WiFi.mode(WIFI_OFF);
    delay(150);
    bool staOk = WiFi.mode(WIFI_STA);
    delay(250);

    if (staOk) {
        WiFi.disconnect(false, false);
        delay(250);
        wh_wifi_configure_country();
        wh_wifi_prepare_scan_dual_band();
        esp_wifi_set_ps(WIFI_PS_NONE);
        wh_wifi_apply_max_tx_power("STA scan");
    }

    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_err_t modeErr = esp_wifi_get_mode(&mode);
    bool ready = staOk && modeErr == ESP_OK && (mode & WIFI_MODE_STA);
    Serial.printf("[S3-WIFI] STA reset off=%d sta=%d modeErr=0x%x mode=%u ready=%d\n",
                  offOk, staOk, modeErr, static_cast<unsigned>(mode), ready);
    return ready;
}

int16_t wh_wifi_scan_networks(bool showHidden,
                              uint32_t maxMsPerChannel,
                              uint8_t channel,
                              bool resetRadio) {
    if (resetRadio && !wh_wifi_prepare_sta_scan()) {
        return WIFI_SCAN_FAILED;
    }

    int16_t count = WiFi.scanNetworks(false, showHidden, false,
                                      maxMsPerChannel, channel);
    if (count >= 0 || !resetRadio) return count;

    // A failed event-driven scan usually means a prior raw ESP-IDF module left
    // the driver between states. Rebuild STA once and retry through Arduino.
    Serial.printf("[S3-WIFI] scan failed=%d; resetting STA and retrying\n", count);
    WiFi.scanDelete();
    if (!wh_wifi_prepare_sta_scan()) return count;
    return WiFi.scanNetworks(false, showHidden, false,
                             maxMsPerChannel, channel);
}

void wh_wifi_print_c5_runtime_report() {
#if WONTHOUND_HAS_WIFI_5G
    const bool wasStarted = (WiFi.getMode() != WIFI_OFF);
    if (!wasStarted) {
        WiFi.mode(WIFI_STA);
        delay(50);
    }

    wh_wifi_configure_dual_band();

    wifi_country_t country = {};
    esp_err_t countryErr = esp_wifi_get_country(&country);

    wifi_band_mode_t bandMode = WIFI_BAND_MODE_2G_ONLY;
    esp_err_t bandErr = esp_wifi_get_band_mode(&bandMode);

    wifi_protocols_t protocols = {};
    esp_err_t protoErr = esp_wifi_get_protocols(WIFI_IF_STA, &protocols);

    wifi_bandwidths_t bandwidths = {};
    esp_err_t bwErr = esp_wifi_get_bandwidths(WIFI_IF_STA, &bandwidths);

    Serial.println("[C5-WIFI] Runtime dual-band fast-sweep report");
    Serial.printf("[C5-WIFI] mode=%s simultaneous=%s single_radio=yes channels=%u 2g=%u 5g=%u\n",
                  wh_wifi_dual_band_mode_label(),
                  WONTHOUND_WIFI_SIMULTANEOUS_DUAL_BAND ? "yes" : "no",
                  wh_wifi_channel_count(),
                  WONTHOUND_WIFI_2G_CHANNEL_COUNT,
                  WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT);
    Serial.println("[C5-WIFI] note=single-radio fast band switching; WiFi/BLE are time-sliced");
    Serial.print("[C5-WIFI] channel sweep:");
    for (uint8_t i = 0; i < wh_wifi_channel_count(); i++) {
        uint8_t ch = wh_wifi_channel_at(i);
        Serial.printf(" %u/%s", ch, wh_wifi_band_label(ch));
    }
    Serial.println();

    if (countryErr == ESP_OK) {
        Serial.printf("[C5-WIFI] country=%c%c schan=%u nchan=%u policy=%u",
                      country.cc[0], country.cc[1],
                      country.schan,
                      country.nchan,
                      (unsigned)country.policy);
#if CONFIG_SOC_WIFI_SUPPORT_5G
        Serial.printf(" 5g_mask=0x%08lX", (unsigned long)country.wifi_5g_channel_mask);
#endif
        Serial.println();
    } else {
        Serial.printf("[C5-WIFI] country read failed: 0x%x\n", countryErr);
    }

    if (bandErr == ESP_OK) {
        Serial.printf("[C5-WIFI] band_mode=%u\n", (unsigned)bandMode);
    } else {
        Serial.printf("[C5-WIFI] band mode read failed: 0x%x\n", bandErr);
    }

    if (protoErr == ESP_OK) {
        Serial.printf("[C5-WIFI] protocols 2g=0x%02X 5g=0x%02X\n",
                      (unsigned)protocols.ghz_2g,
                      (unsigned)protocols.ghz_5g);
    } else {
        Serial.printf("[C5-WIFI] protocol read failed: 0x%x\n", protoErr);
    }

    if (bwErr == ESP_OK) {
        Serial.printf("[C5-WIFI] bandwidths 2g=%u 5g=%u\n",
                      (unsigned)bandwidths.ghz_2g,
                      (unsigned)bandwidths.ghz_5g);
    } else {
        Serial.printf("[C5-WIFI] bandwidth read failed: 0x%x\n", bwErr);
    }

    if (!wasStarted) {
        WiFi.mode(WIFI_OFF);
        delay(20);
    }
#endif
}

esp_err_t wh_wifi_scan_dual_band_records(wifi_ap_record_t* records, uint16_t* count, bool showHidden, uint32_t maxMsPerChan) {
    if (!records || !count || *count == 0) return ESP_ERR_INVALID_ARG;

#if WONTHOUND_HAS_WIFI_5G
    wh_wifi_configure_dual_band();

    wifi_scan_config_t config = {};
    config.channel = 0;
    config.show_hidden = showHidden;
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.min = 60;
    config.scan_time.active.max = maxMsPerChan ? maxMsPerChan : 180;
    config.home_chan_dwell_time = 30;

    for (uint8_t i = 0; i < WONTHOUND_WIFI_2G_CHANNEL_COUNT; i++) {
        config.channel_bitmap.ghz_2_channels |= BIT(WH_WIFI_2G_CHANNELS[i]);
    }
    for (uint8_t i = 0; i < WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT; i++) {
        config.channel_bitmap.ghz_5_channels |= CHANNEL_TO_BIT(WH_WIFI_5G_ACTIVE_CHANNELS[i]);
    }

    esp_err_t err = esp_wifi_scan_start(&config, true);
    if (err != ESP_OK) return err;

    uint16_t found = 0;
    err = esp_wifi_scan_get_ap_num(&found);
    if (err != ESP_OK) return err;

    if (found < *count) *count = found;
    return esp_wifi_scan_get_ap_records(count, records);
#else
    (void)showHidden;
    (void)maxMsPerChan;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wh_esp_wifi_set_channel(uint8_t channel, wifi_second_chan_t second) {
#if WONTHOUND_HAS_WIFI_5G
    channel = wh_normalize_wifi_channel(channel);
    wh_wifi_prepare_channel(channel);
#endif
    return esp_wifi_set_channel(channel, second);
}
