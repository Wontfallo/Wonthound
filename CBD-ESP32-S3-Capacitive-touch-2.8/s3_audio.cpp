#include "s3_audio.h"

#if defined(ESP32S3_ES3C28P) && CYD_HAS_AUDIO

#include "touch_buttons.h"
#include "shared.h"
#include "utils.h"
#include <TFT_eSPI.h>
#include <Wire.h>
#include <driver/i2s.h>

extern TFT_eSPI tft;
extern void drawInoIconBar();

static constexpr uint8_t ES8311_ADDR = 0x18;
static constexpr uint32_t AUDIO_SAMPLE_RATE = 16000;
static bool audioReady = false;

static bool es8311Write(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static bool es8311Probe() {
    Wire.beginTransmission(ES8311_ADDR);
    return Wire.endTransmission() == 0;
}

static bool es8311InitMinimal() {
    if (!es8311Probe()) return false;

    // Minimal ES8311 bring-up, matching the vendor example's 16 kHz analog mic path.
    es8311Write(0x00, 0x1F);
    delay(10);
    es8311Write(0x00, 0x00);
    es8311Write(0x00, 0x80);
    delay(10);

    es8311Write(0x01, 0x30);
    es8311Write(0x02, 0x10);
    es8311Write(0x03, 0x10);
    es8311Write(0x04, 0x10);
    es8311Write(0x05, 0x00);
    es8311Write(0x06, 0x20);
    es8311Write(0x07, 0x00);
    es8311Write(0x08, 0xFF);
    es8311Write(0x09, 0x0C);
    es8311Write(0x0A, 0x0C);
    es8311Write(0x0D, 0x01);
    es8311Write(0x0E, 0x02);
    es8311Write(0x12, 0x00);
    es8311Write(0x13, 0x10);
    es8311Write(0x14, 0x1A);
    es8311Write(0x16, 0x00);
    es8311Write(0x17, 0xC8);
    es8311Write(0x1C, 0x6A);
    es8311Write(0x31, 0x00);
    es8311Write(0x32, 0xBF);
    es8311Write(0x37, 0x08);
    return true;
}

bool s3AudioBegin() {
    if (audioReady) return true;

    Wire.begin(ES8311_SDA, ES8311_SCL);
    Wire.setClock(400000);

    pinMode(ES8311_PA_EN, OUTPUT);
    digitalWrite(ES8311_PA_EN, HIGH);

    if (!es8311InitMinimal()) {
        Serial.println("[AUDIO] ES8311 not found on I2C");
        return false;
    }

    i2s_config_t i2sCfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = AUDIO_SAMPLE_RATE * 384,
        .mclk_multiple = I2S_MCLK_MULTIPLE_384,
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2sCfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] i2s_driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    i2s_pin_config_t pinCfg = {
        .mck_io_num = ES8311_MCLK,
        .bck_io_num = ES8311_BCLK,
        .ws_io_num = ES8311_LRCLK,
        .data_out_num = ES8311_DOUT,
        .data_in_num = ES8311_DIN,
    };

    err = i2s_set_pin(I2S_NUM_1, &pinCfg);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] i2s_set_pin failed: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_NUM_1);
        return false;
    }

    audioReady = true;
    Serial.println("[AUDIO] ES8311 + I2S ready");
    return true;
}

bool s3AudioReady() {
    return audioReady;
}

int s3AudioReadLevel(uint32_t windowMs) {
    if (!audioReady && !s3AudioBegin()) return -1;

    int16_t samples[128];
    uint32_t start = millis();
    uint32_t sum = 0;
    uint32_t count = 0;

    while (millis() - start < windowMs) {
        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_NUM_1, samples, sizeof(samples), &bytesRead, 20 / portTICK_PERIOD_MS);
        if (err != ESP_OK || bytesRead == 0) continue;

        size_t sampleCount = bytesRead / sizeof(samples[0]);
        for (size_t i = 0; i < sampleCount; i++) {
            sum += abs(samples[i]);
        }
        count += sampleCount;
    }

    if (count == 0) return 0;
    return (int)min<uint32_t>(4095, sum / count);
}

void s3AudioTestScreen() {
    tft.fillScreen(TFT_BLACK);
    drawStatusBar();
    drawInoIconBar();
    drawGlitchTitle(58, "S3 AUDIO");

    bool ok = s3AudioBegin();
    tft.setTextSize(1);
    drawCenteredText(92, ok ? "ES8311 + I2S READY" : "AUDIO INIT FAILED", ok ? WONTHOUND_HOTPINK : TFT_RED, 1);
    drawCenteredText(110, "Tap back or press BOOT", WONTHOUND_GUNMETAL, 1);

    while (true) {
        touchButtonsUpdate();
        if (isBackButtonTapped() || IS_BOOT_PRESSED()) {
            waitForTouchRelease();
            break;
        }

        int level = ok ? s3AudioReadLevel(80) : -1;
        int barX = 25;
        int barY = 165;
        int barW = SCREEN_WIDTH - 50;
        int fillW = level > 0 ? map(min(level, 1600), 0, 1600, 0, barW) : 0;

        tft.fillRect(barX, barY - 30, barW, 70, TFT_BLACK);
        tft.drawRect(barX, barY, barW, 18, WONTHOUND_MAGENTA);
        tft.fillRect(barX + 1, barY + 1, max(0, fillW - 2), 16, WONTHOUND_HOTPINK);
        tft.setTextColor(WONTHOUND_BRIGHT, TFT_BLACK);
        tft.setCursor(barX, barY - 18);
        tft.printf("Mic level: %d   ", level);
        delay(20);
    }
}

#else

bool s3AudioBegin() { return false; }
bool s3AudioReady() { return false; }
int s3AudioReadLevel(uint32_t windowMs) { (void)windowMs; return -1; }
void s3AudioTestScreen() {}

#endif
