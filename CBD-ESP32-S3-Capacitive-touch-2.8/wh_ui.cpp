// ═══════════════════════════════════════════════════════════════════════════
// WontHound shared UI toolkit — implementation. See wh_ui.h.
// ═══════════════════════════════════════════════════════════════════════════

#include "wh_ui.h"
#include "utils.h"
#include "touch_buttons.h"

#define WH_RED 0xF800   // danger accent (RGB565 red)

// Map a button state to its (fill, border, text) colors.
static void whStateColors(WhBtnState st, uint16_t& fill, uint16_t& border, uint16_t& fg) {
    switch (st) {
        case WH_ON:        fill = WONTHOUND_HOTPINK; border = TFT_WHITE;        fg = TFT_WHITE;        break;
        case WH_ACCENT:    fill = WONTHOUND_DARK;    border = WONTHOUND_HOTPINK; fg = WONTHOUND_HOTPINK; break;
        case WH_DANGER:    fill = WONTHOUND_DARK;    border = WH_RED;           fg = WH_RED;           break;
        case WH_DANGER_ON: fill = WH_RED;            border = TFT_WHITE;        fg = TFT_WHITE;        break;
        case WH_OUTLINE:
        default:           fill = WONTHOUND_DARK;    border = WONTHOUND_MAGENTA; fg = WONTHOUND_MAGENTA; break;
    }
}

WhRect whBackRect() {
    WhRect r = { 6, 5, 60, 24 };
    return r;
}

WhRect whDrawHeaderBand(const char* title) {
    int h = WH_HEADER_H;
    tft.fillRect(0, 0, SCREEN_WIDTH, h, WONTHOUND_DARK);
    tft.drawFastHLine(0, h - 2, SCREEN_WIDTH, WONTHOUND_VIOLET);
    tft.drawFastHLine(0, h - 1, SCREEN_WIDTH, WONTHOUND_HOTPINK);

    WhRect b = whBackRect();
    tft.fillRoundRect(b.x, b.y, b.w, b.h, 7, WONTHOUND_BLACK);
    tft.drawRoundRect(b.x, b.y, b.w, b.h, 7, WONTHOUND_HOTPINK);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WONTHOUND_HOTPINK);
    tft.drawString("< Back", b.x + b.w / 2, b.y + b.h / 2 + 1);

    if (title) {
        tft.setTextColor(WONTHOUND_HOTPINK);
        tft.drawString(title, SCREEN_WIDTH / 2, h / 2 + 1);
    }
    tft.setTextDatum(TL_DATUM);

#if CYD_HAS_GPS
    drawGPSIndicator(SCREEN_WIDTH - 58, 8);
#endif
    return b;
}

void whActionButton(const WhRect& r, const char* label, const unsigned char* icon, WhBtnState st) {
    uint16_t fill, border, fg;
    whStateColors(st, fill, border, fg);
    tft.fillRoundRect(r.x, r.y, r.w, r.h, 8, fill);
    tft.drawRoundRect(r.x, r.y, r.w, r.h, 8, border);
    int cx = r.x + r.w / 2;
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(fg);
    if (icon) {
        tft.drawBitmap(cx - 8, r.y + 8, icon, 16, 16, fg);
        tft.drawString(label, cx, r.y + r.h - 12);
    } else {
        tft.drawString(label, cx, r.y + r.h / 2);
    }
    tft.setTextDatum(TL_DATUM);
}

void whLayoutBar(int count, WhRect* out) {
    if (count < 1) return;
    const int margin = 8, gap = 6;
    int total = SCREEN_WIDTH - 2 * margin - (count - 1) * gap;
    int w = total / count;
    int x = margin;
    for (int i = 0; i < count; i++) {
        // Last button soaks up any rounding remainder so the row stays flush-right.
        int bw = (i == count - 1) ? (SCREEN_WIDTH - margin - x) : w;
        out[i] = { (int16_t)x, (int16_t)WH_BAR_Y, (int16_t)bw, (int16_t)WH_BAR_H };
        x += bw + gap;
    }
}

void whStatusChip(int x, int y, int w, const char* label, const char* value,
                  uint16_t border, uint16_t valColor) {
    const int h = 22;
    tft.fillRoundRect(x, y, w, h, 6, WONTHOUND_DARK);
    tft.drawRoundRect(x, y, w, h, 6, border);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WONTHOUND_MAGENTA);
    int lx = x + 7;
    tft.drawString(label, lx, y + h / 2);
    tft.setTextColor(valColor);
    tft.drawString(value, lx + tft.textWidth(label) + (label[0] ? 4 : 0), y + h / 2);
    tft.setTextDatum(TL_DATUM);
}

bool whHit(uint16_t tx, uint16_t ty, const WhRect& r) {
    return (int)tx >= r.x && (int)tx <= r.x + r.w &&
           (int)ty >= r.y && (int)ty <= r.y + r.h;
}

// ── Top control bar ─────────────────────────────────────────────────────────
#define WH_BACK_W 64   // matches the fixed Back touch zone (TOUCH_BTN_BACK_X2)

WhRect whBackButtonRect() {
    WhRect r = { 0, 0, WH_BACK_W, (int16_t)WH_TOPBAR_H };
    return r;
}

void whDrawBackButton() {
    tft.fillRect(0, 0, SCREEN_WIDTH, WH_TOPBAR_H, WONTHOUND_BLACK);
    int h = WH_TOPBAR_H;
    tft.fillRoundRect(2, 2, WH_BACK_W - 4, h - 4, 6, WONTHOUND_BLACK);
    tft.drawRoundRect(2, 2, WH_BACK_W - 4, h - 4, 6, WONTHOUND_HOTPINK);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WONTHOUND_HOTPINK);
    tft.drawString("< BACK", WH_BACK_W / 2, h / 2);
    tft.setTextDatum(TL_DATUM);
}

void whTopBarActions(int count, WhRect* out) {
    if (count < 1) return;
    const int gap = 3;
    int x0 = WH_BACK_W + 2;
    int total = SCREEN_WIDTH - x0 - 2 - (count - 1) * gap;
    int w = total / count;
    int x = x0;
    for (int i = 0; i < count; i++) {
        int bw = (i == count - 1) ? (SCREEN_WIDTH - 2 - x) : w;
        out[i] = { (int16_t)x, 2, (int16_t)bw, (int16_t)(WH_TOPBAR_H - 4) };
        x += bw + gap;
    }
}

void whTopBtn(const WhRect& r, const char* label, WhBtnState st) {
    uint16_t fill, border, fg;
    whStateColors(st, fill, border, fg);
    tft.fillRoundRect(r.x, r.y, r.w, r.h, 6, fill);
    tft.drawRoundRect(r.x, r.y, r.w, r.h, 6, border);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(fg);
    tft.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
    tft.setTextDatum(TL_DATUM);
}
