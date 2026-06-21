#ifndef SHARED_H
#define SHARED_H

// ═══════════════════════════════════════════════════════════════════════════
// WontHound-CYD Shared Definitions
// Color palette and shared state variables
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// WONTHOUND COLOR PALETTE
// Jesse's Custom: Red/Purple/Pink theme (no yellow/orange)
// Theme colors are extern — runtime-adjustable for colorblind modes
// ═══════════════════════════════════════════════════════════════════════════

// Theme colors — changed by applyColorMode() at runtime
extern uint16_t WONTHOUND_MAGENTA;   // Primary (selected items)
extern uint16_t WONTHOUND_HOTPINK;   // Accents
extern uint16_t WONTHOUND_BRIGHT;    // Highlights
extern uint16_t WONTHOUND_VIOLET;    // Accent color
extern uint16_t WONTHOUND_CYAN;      // Text color
extern uint16_t WONTHOUND_GREEN;     // Secondary accent

// Fixed colors — never change with color mode
const uint16_t WONTHOUND_DARK = 0x2841;     // #2B080A - Dark backgrounds
const uint16_t WONTHOUND_BLACK = 0x0000;    // #000000 - Pure black
const uint16_t WONTHOUND_GUNMETAL = 0x18E3; // #1C1C1C - Gunmetal gray

// ═══════════════════════════════════════════════════════════════════════════
// LEGACY COLOR MAPPINGS (for compatibility with original code)
// Macros so they follow theme color changes at runtime
// ═══════════════════════════════════════════════════════════════════════════

#define SHREDDY_TEAL    WONTHOUND_MAGENTA
#define SHREDDY_PINK    WONTHOUND_MAGENTA
#define SHREDDY_BLACK   WONTHOUND_BLACK
#define SHREDDY_BLUE    WONTHOUND_MAGENTA
#define SHREDDY_PURPLE  WONTHOUND_VIOLET
#define SHREDDY_GREEN   WONTHOUND_GREEN
#define SHREDDY_GUNMETAL WONTHOUND_GUNMETAL

// Standard colors
#define ORANGE          WONTHOUND_MAGENTA
const uint16_t GRAY = 0x8410;
#define BLUE            WONTHOUND_MAGENTA
const uint16_t RED = 0xF800;
#define GREEN           WONTHOUND_GREEN
const uint16_t BLACK = 0x0000;
const uint16_t WHITE = 0xFFFF;
const uint16_t LIGHT_GRAY = 0xC618;
const uint16_t DARK_GRAY = WONTHOUND_GUNMETAL;

// TFT color defines
#define TFT_DARKBLUE  0x3166
#define TFT_LIGHTBLUE WONTHOUND_MAGENTA
#define TFTWHITE     0xFFFF
#define TFT_GRAY      0x8410
#define SELECTED_ICON_COLOR WONTHOUND_MAGENTA

// ═══════════════════════════════════════════════════════════════════════════
// MENU STATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════

extern bool in_sub_menu;
extern bool feature_active;
extern bool submenu_initialized;
extern bool is_main_menu;
extern bool feature_exit_requested;

// ═══════════════════════════════════════════════════════════════════════════
// CYD-SPECIFIC STATE
// ═══════════════════════════════════════════════════════════════════════════

extern bool gps_enabled;
extern bool gps_has_fix;

// LEGAL DISCLAIMER STATE
extern bool disclaimer_accepted;    // Legal disclaimer accepted (persisted in EEPROM)

// CC1101 PA MODULE STATE
extern bool cc1101_pa_module;       // E07-433M20S PA module active (persisted in EEPROM)

// WONTHOUND-ALPHA (C5 CO-PROCESSOR) STATE
extern bool c5_connected;           // C5 co-processor detected on P1 UART

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════

void displaySubmenu();

// ═══════════════════════════════════════════════════════════════════════════
// SCREEN DIMENSIONS (from cyd_config.h)
// ═══════════════════════════════════════════════════════════════════════════

#define SCREEN_WIDTH  CYD_SCREEN_WIDTH
#define SCREEN_HEIGHT CYD_SCREEN_HEIGHT

// ═══════════════════════════════════════════════════════════════════════════
// STATUS BAR HEIGHT (for GPS indicator, battery, etc.)
// ═══════════════════════════════════════════════════════════════════════════

#define STATUS_BAR_HEIGHT 0

#endif // SHARED_H
