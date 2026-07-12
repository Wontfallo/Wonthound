#include "radio_power_utils.h"

#include <BLEDevice.h>
#include <esp_bt.h>
#include <esp_err.h>
#include <esp_wifi.h>

static const char* wh_power_context(const char* context) {
    return (context && context[0]) ? context : "radio";
}

bool wh_wifi_apply_max_tx_power(const char* context) {
    const esp_err_t setErr = esp_wifi_set_max_tx_power(WONTHOUND_WIFI_MAX_TX_POWER_QDBM);
    int8_t accepted = 0;
    const esp_err_t getErr = esp_wifi_get_max_tx_power(&accepted);

    wifi_country_t country = {};
    const esp_err_t countryErr = esp_wifi_get_country(&country);
    Serial.printf("[S3-RF] WiFi %s set=0x%x get=0x%x requested=20.00dBm accepted=%.2fdBm",
                  wh_power_context(context), setErr, getErr,
                  getErr == ESP_OK ? accepted / 4.0f : -1.0f);
    if (countryErr == ESP_OK) {
        Serial.printf(" country=%c%c channels=%u-%u limit=%ddBm",
                      country.cc[0], country.cc[1], country.schan,
                      country.schan + country.nchan - 1, country.max_tx_power);
    }
    Serial.println();

    return setErr == ESP_OK && getErr == ESP_OK &&
           accepted == WONTHOUND_WIFI_MAX_TX_POWER_QDBM;
}

bool wh_ble_apply_max_tx_power(const char* context) {
    esp_err_t defaultErr = ESP_FAIL;
    esp_err_t advErr = ESP_FAIL;
    esp_err_t scanErr = ESP_FAIL;
    esp_power_level_t defaultLevel = ESP_PWR_LVL_INVALID;
    esp_power_level_t advLevel = ESP_PWR_LVL_INVALID;
    esp_power_level_t scanLevel = ESP_PWR_LVL_INVALID;

    for (int attempt = 0; attempt < 3; attempt++) {
        defaultErr = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P21);
        advErr = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P21);
        scanErr = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P21);
        defaultLevel = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
        advLevel = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV);
        scanLevel = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN);
        if (defaultErr == ESP_OK && advErr == ESP_OK && scanErr == ESP_OK &&
            defaultLevel == ESP_PWR_LVL_P21 && advLevel == ESP_PWR_LVL_P21 &&
            scanLevel == ESP_PWR_LVL_P21) {
            break;
        }
        delay(25);
    }

    const bool ok = defaultErr == ESP_OK && advErr == ESP_OK && scanErr == ESP_OK &&
                    defaultLevel == ESP_PWR_LVL_P21 && advLevel == ESP_PWR_LVL_P21 &&
                    scanLevel == ESP_PWR_LVL_P21;
    Serial.printf("[S3-RF] BLE %s max=%s set=0x%x/0x%x/0x%x read=%d/%d/%d target<=20dBm\n",
                  wh_power_context(context), ok ? "accepted" : "FAILED",
                  defaultErr, advErr, scanErr,
                  static_cast<int>(defaultLevel), static_cast<int>(advLevel),
                  static_cast<int>(scanLevel));
    return ok;
}

void wh_ble_init_max_power(const std::string& deviceName) {
    BLEDevice::init(deviceName);
    wh_ble_apply_max_tx_power(deviceName.empty() ? "anonymous" : deviceName.c_str());
}
