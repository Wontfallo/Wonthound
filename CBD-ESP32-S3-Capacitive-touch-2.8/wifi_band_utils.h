#ifndef WIFI_BAND_UTILS_H
#define WIFI_BAND_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "cyd_config.h"

#if defined(ESP32C5_NM_CYD) || defined(CONFIG_IDF_TARGET_ESP32C5) || defined(ARDUINO_ARCH_ESP32C5)
  #define WONTHOUND_HAS_WIFI_5G 1
#else
#define WONTHOUND_HAS_WIFI_5G 0
#endif

#define WONTHOUND_WIFI_SIMULTANEOUS_DUAL_BAND 0
#define WONTHOUND_WIFI_DUAL_BAND_FAST_SWEEP 1
#define WONTHOUND_WIFI_2G_CHANNEL_COUNT 11
#define WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT 25

extern const uint8_t WH_WIFI_2G_CHANNELS[WONTHOUND_WIFI_2G_CHANNEL_COUNT];

#if WONTHOUND_HAS_WIFI_5G
extern const uint8_t WH_WIFI_5G_ACTIVE_CHANNELS[WONTHOUND_WIFI_5G_ACTIVE_CHANNEL_COUNT];
#endif

bool wh_wifi_is_5g_channel(uint8_t channel);
const char* wh_wifi_band_label(uint8_t channel);
const char* wh_wifi_dual_band_mode_label();
uint8_t wh_wifi_channel_count();
uint8_t wh_wifi_channel_at(uint8_t index);
uint8_t wh_normalize_wifi_channel(int channel);
uint8_t wh_next_wifi_channel(uint8_t current, int direction);
uint8_t wh_random_wifi_channel();
void wh_wifi_configure_country();
void wh_wifi_prepare_scan_dual_band();
void wh_wifi_prepare_channel(uint8_t channel);
void wh_wifi_prepare_ap_channel(uint8_t channel);
void wh_wifi_configure_dual_band();
bool wh_wifi_prepare_sta_scan();
int16_t wh_wifi_scan_networks(bool showHidden = true,
                              uint32_t maxMsPerChannel = 300,
                              uint8_t channel = 0,
                              bool resetRadio = true);
void wh_wifi_print_c5_runtime_report();
esp_err_t wh_wifi_scan_dual_band_records(wifi_ap_record_t* records, uint16_t* count, bool showHidden, uint32_t maxMsPerChan);
esp_err_t wh_esp_wifi_set_channel(uint8_t channel, wifi_second_chan_t second);

#ifndef WONTHOUND_WIFI_BAND_UTILS_IMPL
  #define esp_wifi_set_channel(channel, second) wh_esp_wifi_set_channel((channel), (second))
#endif

#endif
