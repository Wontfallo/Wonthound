#ifndef RADIO_POWER_UTILS_H
#define RADIO_POWER_UTILS_H

#include <Arduino.h>
#include <string>

// IDF 4.4 maps 80 quarter-dBm units to its highest WiFi setting: 20 dBm.
constexpr int8_t WONTHOUND_WIFI_MAX_TX_POWER_QDBM = 80;

bool wh_wifi_apply_max_tx_power(const char* context);
bool wh_ble_apply_max_tx_power(const char* context);
void wh_ble_init_max_power(const std::string& deviceName);

#endif
