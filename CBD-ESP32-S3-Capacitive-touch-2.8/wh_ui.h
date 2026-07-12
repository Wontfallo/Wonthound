#ifndef WH_UI_H
#define WH_UI_H

// ═══════════════════════════════════════════════════════════════════════════
// WontHound shared UI toolkit — big, thumb-friendly touch chrome.
//
// Replaces the legacy "tiny 16px top icon bar" that every feature screen used.
// These primitives reproduce the Beacon Spammer redesign so every screen can
// adopt the SAME look with a few calls instead of hand-placing 16px bitmaps:
//
//   • whDrawHeaderBand()  — top band + big "< Back" pill + live GPS chip
//   • whActionButton()    — one large rounded button (icon over label)
//   • whLayoutBar()       — evenly lays N buttons across the bottom action bar
//   • whStatusChip()      — read-only rounded label/value chip
//   • whHit()             — rect hit-test for the big targets
//
// Front-end only: no radio / capture / state logic lives here.
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "shared.h"

// Visual state of an action button (drives fill / border / text colors).
enum WhBtnState {
    WH_OUTLINE,     // neutral outlined button (magenta)
    WH_ACCENT,      // idle primary (hot-pink outline)
    WH_ON,          // active / live primary (solid hot-pink)
    WH_DANGER,      // destructive, idle (red outline)
    WH_DANGER_ON    // destructive, active (solid red)
};

// A rectangle used for both drawing and touch hit-testing.
struct WhRect { int16_t x, y, w, h; };

// Shared layout metrics (240x320 base, auto-scaled on other panels).
#define WH_HEADER_H   SCALE_Y(34)                       // top header-band height
#define WH_BAR_H      SCALE_Y(54)                        // bottom action-bar button height
#define WH_BAR_Y      (SCREEN_HEIGHT - WH_BAR_H - 4)     // bottom action-bar top edge

// Top header band: dark band + double accent rule + prominent "< Back" pill on
// the left (inside the fixed back touch zone) + live GPS chip on the right.
// When `title` is non-null it is centered in the band. Returns the Back rect.
WhRect whDrawHeaderBand(const char* title);

// The Back-pill hit rect (top-left), matching whDrawHeaderBand().
WhRect whBackRect();

// One large action button. When `icon` (16x16 monochrome bitmap) is non-null it
// is stacked above the label; otherwise the label is centered.
void whActionButton(const WhRect& r, const char* label, const unsigned char* icon, WhBtnState st);

// Evenly lay out `count` buttons across the bottom action bar into out[0..count).
void whLayoutBar(int count, WhRect* out);

// Read-only status chip: rounded frame with "label value" (e.g. "CH 6").
void whStatusChip(int x, int y, int w, const char* label, const char* value,
                  uint16_t border, uint16_t valColor);

// Rectangular hit-test in screen coordinates.
bool whHit(uint16_t tx, uint16_t ty, const WhRect& r);

// ── Top control bar ─────────────────────────────────────────────────────────
// A short top strip with a DEDICATED Back button that owns the entire fixed
// top-left Back touch zone (x<64), plus N action buttons spread across the rest
// — so no action button ever overlaps the Back zone (the resistive-panel bug).
#define WH_TOPBAR_H   SCALE_Y(36)

// Fill the strip and draw the Back button (top-left). Call first.
void whDrawBackButton();
// The Back button hit rect (covers the full fixed Back zone).
WhRect whBackButtonRect();
// Lay out `count` action-button rects across [64 .. SCREEN_WIDTH) into out[].
void whTopBarActions(int count, WhRect* out);
// Draw one top-bar action button (compact font, fits narrow slots).
void whTopBtn(const WhRect& r, const char* label, WhBtnState st);

#endif  // WH_UI_H
