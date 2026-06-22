// ═══════════════════════════════════════════════════════════════════════════
// WontHound-CYD User_Setup.h
// TFT_eSPI Configuration for Cheap Yellow Display Boards
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════
//
// BOARD SELECTION: Set by PlatformIO build flags (-DCYD_E32R28T=1, -DCYD_E32R35T=1)
// Default: CYD_28 when no flag specified (backwards compatible)
// ═══════════════════════════════════════════════════════════════════════════

// Freenove E32R28T P3 NRF24 target inherits CYD_28 display/touch.
#ifdef FREENOVE_E32R28T_P3_NRF24
  #ifndef CYD_E32R28T
    #define CYD_E32R28T
  #endif
#endif

// E32R28T inherits CYD_28
#ifdef CYD_E32R28T
  #ifndef CYD_28
    #define CYD_28
  #endif
#endif

// E32R35T inherits CYD_35
#ifdef CYD_E32R35T
  #ifndef CYD_35
    #define CYD_35
  #endif
#endif

// ESP32-3248S035C inherits CYD_35 (same ST7796 display)
#ifdef CYD_3248S035C
  #ifndef CYD_35
    #define CYD_35
  #endif
  #ifndef CYD_CAP_TOUCH
    #define CYD_CAP_TOUCH
  #endif
#endif

// ESP32-S3 ES3C28P/ES3N28P: 2.8" ILI9341V with FT6336 capacitive touch.
#ifdef ESP32S3_ES3C28P
  #ifndef CYD_28
    #define CYD_28
  #endif
  #ifndef CYD_CAP_TOUCH
    #define CYD_CAP_TOUCH
  #endif
  #ifndef CYD_FT6336_TOUCH
    #define CYD_FT6336_TOUCH
  #endif
#endif

// NM-CYD-C5: 2.8" ST7789 with XPT2046 resistive touch on shared SPI.
#ifdef ESP32C5_NM_CYD
  #ifndef CYD_28
    #define CYD_28
  #endif
  #ifndef CYD_SHARED_XPT2046_TOUCH
    #define CYD_SHARED_XPT2046_TOUCH
  #endif
#endif

#if !defined(CYD_28) && !defined(CYD_35)
  #define CYD_28    // Default: ESP32-2432S028 - 2.8" 320x240 ILI9341
#endif

// ═══════════════════════════════════════════════════════════════════════════
// USER DEFINED SETTINGS - TFT_eSPI Library Configuration
// ═══════════════════════════════════════════════════════════════════════════

#define USER_SETUP_INFO "WontHound-CYD"

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 1: DISPLAY DRIVER SELECTION
// ═══════════════════════════════════════════════════════════════════════════

#ifdef ESP32C5_NM_CYD
  // NM-CYD-C5 2.8" board uses ST7789.
  #define ST7789_DRIVER
  #define TFT_WIDTH  240
  #define TFT_HEIGHT 320
  #define TFT_INVERSION_OFF
  #define CYD_BOARD_NAME "NM-CYD-C5 ESP32-C5 2.8\""
#elif defined(ESP32S3_ES3C28P)
  // 2.8" ESP32-S3 module uses ILI9341V.
  #define ILI9341_DRIVER
  #define TFT_WIDTH  240
  #define TFT_HEIGHT 320
  #define CYD_BOARD_NAME "ESP32-S3 ES3C28P 2.8\""
#elif defined(CYD_28)
  // 2.8" CYD uses ILI9341 variant 2 (handles more panel revisions correctly)
  #define ILI9341_2_DRIVER
  #define TFT_WIDTH  240
  #define TFT_HEIGHT 320
  #define CYD_BOARD_NAME "CYD 2.8\" (ESP32-2432S028)"
#endif

#ifdef CYD_35
  // E32R35T 3.5" uses ST7796
  #define ST7796_DRIVER
  #define TFT_WIDTH  320
  #define TFT_HEIGHT 480
  #define CYD_BOARD_NAME "E32R35T 3.5\" (QDtech E32R35T)"
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 2: DISPLAY SPI PINS (HSPI)
// Same for both boards
// ═══════════════════════════════════════════════════════════════════════════

#ifdef ESP32C5_NM_CYD
  #define TFT_MISO 2
  #define TFT_MOSI 7
  #define TFT_SCLK 6
  #define TFT_CS   23
  #define TFT_DC   24
  #define TFT_RST  -1   // LCD reset tied to ESP reset
#elif defined(ESP32S3_ES3C28P)
  #define TFT_MISO 13
  #define TFT_MOSI 11
  #define TFT_SCLK 12
  #define TFT_CS   10
  #define TFT_DC   46
  #define TFT_RST  -1   // LCD reset tied to ESP reset
#else
  #define TFT_MISO 12
  #define TFT_MOSI 13
  #define TFT_SCLK 14
  #define TFT_CS   15
  #define TFT_DC    2
  #define TFT_RST  -1   // Connected to EN/RST on both boards
#endif

// Backlight pin differs between boards
#ifdef ESP32C5_NM_CYD
  #define TFT_BL   25   // NM-CYD-C5 backlight
#elif defined(ESP32S3_ES3C28P)
  #define TFT_BL   45   // ESP32-S3 ES3C28P backlight
#elif defined(CYD_28)
  #define TFT_BL   21   // 2.8" backlight on GPIO21
#endif
#ifdef CYD_35
  #define TFT_BL   27   // 3.5" backlight on GPIO27
#endif

#define TFT_BACKLIGHT_ON HIGH

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 2B: TOUCH CONTROLLER PINS
// ═══════════════════════════════════════════════════════════════════════════

// Touch chip select — XPT2046 on resistive touch boards only
// CYD 2.8": separate SPI bus (bit-banged)
// E32R35T 3.5": shared HSPI with LCD (TFT_eSPI built-in driver)
// CYD35C 3.5": GT911 capacitive touch (I2C) — GPIO 33 is GT911 SDA, NOT XPT2046 CS
#if defined(ESP32C5_NM_CYD)
  #define TOUCH_CS 1
#elif !defined(CYD_CAP_TOUCH)
  #define TOUCH_CS 33
#endif

#if defined(ESP32C5_NM_CYD)
  // NM-CYD-C5 XPT2046 shares the ST7789 SPI bus and CS is GPIO1.
  #define CYD_TOUCH_IRQ   -1
  #define CYD_TOUCH_SEPARATE_SPI 0
#elif defined(CYD_28) && !defined(CYD_CAP_TOUCH)
  // 2.8" has DEDICATED touch SPI bus (separate from display)
  #define CYD_TOUCH_IRQ   36
  #define CYD_TOUCH_MOSI  32
  #define CYD_TOUCH_MISO  39
  #define CYD_TOUCH_CLK   25
  #define CYD_TOUCH_SEPARATE_SPI 1
#endif

#ifdef CYD_35
  // E32R35T: XPT2046 resistive touch shares HSPI with LCD (GPIO 14/13/12)
  // TFT_eSPI built-in driver handles touch via TOUCH_CS define above
  #define CYD_TOUCH_SEPARATE_SPI 0
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 3: FONTS
// ═══════════════════════════════════════════════════════════════════════════

#define LOAD_GLCD    // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2   // Font 2. Small 16 pixel high font
#define LOAD_FONT4   // Font 4. Medium 26 pixel high font
#define LOAD_FONT6   // Font 6. Large 48 pixel font (numbers only)
#define LOAD_FONT7   // Font 7. 7 segment 48 pixel font (numbers only)
#define LOAD_FONT8   // Font 8. Large 75 pixel font (numbers only)
#define LOAD_GFXFF   // FreeFonts - 48 Adafruit_GFX free fonts

#define SMOOTH_FONT

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 4: SPI SETTINGS
// ═══════════════════════════════════════════════════════════════════════════

// Display SPI frequency
#ifdef ESP32C5_NM_CYD
  #define SPI_FREQUENCY  20000000  // NM reference setup drives ST7789 at 20MHz
#elif defined(ESP32S3_ES3C28P)
  #define SPI_FREQUENCY  80000000  // Vendor examples drive ILI9341V at 80MHz
#elif defined(CYD_28)
  #define SPI_FREQUENCY  40000000  // 40MHz — matches Bruce, works on all CYD panel variants
#endif
#ifdef CYD_35
  #define SPI_FREQUENCY  40000000  // 40MHz for ST7796 (more conservative)
#endif

// Touch SPI frequency (XPT2046 requires slower speed)
#define SPI_TOUCH_FREQUENCY  2500000  // 2.5MHz

// Read frequency
#define SPI_READ_FREQUENCY  16000000  // 16MHz — matches Bruce, safer for all panel variants

// Use HSPI port for display. On ESP32-S3 this keeps TFT_eSPI off Arduino's
// default SPI object, leaving that bus free for NRF24 on the P3 header.
#ifndef ESP32C5_NM_CYD
  #define USE_HSPI_PORT
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 5: ADDITIONAL SETTINGS
// ═══════════════════════════════════════════════════════════════════════════

// Color order - try BGR if colors look wrong
//#define TFT_RGB_ORDER TFT_RGB
//#define TFT_RGB_ORDER TFT_BGR

// Inversion - uncomment if display colors are inverted
//#define TFT_INVERSION_ON
//#define TFT_INVERSION_OFF

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 6: WONTHOUND-CYD SPECIFIC PINS (from cyd_config.h)
// Defined here for reference - actual usage in cyd_config.h
// ═══════════════════════════════════════════════════════════════════════════

// These pins are the SAME on both 2.8" and 3.5" boards:
//
// SD Card (VSPI) - DISABLED, used for radios:
//   SD_CS   = GPIO 5
//   SD_SCK  = GPIO 18  → Used for CC1101/NRF24 SPI
//   SD_MISO = GPIO 19  → Used for CC1101/NRF24 SPI
//   SD_MOSI = GPIO 23  → Used for CC1101/NRF24 SPI
//
// RGB LED - DISABLED, used for NRF24:
//   RED   = GPIO 4   → NRF24_CSN
//   GREEN = GPIO 16  → NRF24_CE
//   BLUE  = GPIO 17  → NRF24_IRQ / GPS_TX
//
// Speaker - DISABLED, used for GPS:
//   SPEAKER = GPIO 26 → GPS_RX_PIN
//
// LDR Sensor (available):
//   LDR = GPIO 34 (input only)

// ═══════════════════════════════════════════════════════════════════════════
// BOARD VALIDATION
// ═══════════════════════════════════════════════════════════════════════════

#if !defined(CYD_28) && !defined(CYD_35)
  #error "You must define either CYD_28 or CYD_35 at the top of User_Setup.h"
#endif

#if defined(CYD_28) && defined(CYD_35)
  #error "You cannot define both CYD_28 and CYD_35 - choose one board"
#endif
