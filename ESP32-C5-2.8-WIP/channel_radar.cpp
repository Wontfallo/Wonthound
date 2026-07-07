// ═══════════════════════════════════════════════════════════════════════════
// WiFi Channel Occupancy — "Airwaves"   (ESP32-C5, dual-band)
// Two stacked spectrum plots on one screen: 2.4GHz on top, 5GHz on the bottom.
// Every AP is a translucent bell (arc) whose height is its signal strength and
// whose width is its channel bleed. Overlapping APs blend, so congested
// channels visibly stack up. Tap an arc for full details; PREV / NEXT scroll.
//
// LIVE scan: one channel is passively swept per loop tick (touch stays live),
// APs age out when not heard, and the plots repaint each full sweep. Only a
// reduced US channel set is hopped to keep the sweep fast. Passive only — no
// probe requests, no transmit.
// ═══════════════════════════════════════════════════════════════════════════

#include "channel_radar.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <math.h>
#include <string.h>

#include "cyd_config.h"
#include "shared.h"
#include "touch_buttons.h"
#include "utils.h"
#include "wh_ui.h"
#include "wifi_band_utils.h"

namespace ChannelRadar {

// ── AP table ────────────────────────────────────────────────────────────────
// Dense areas easily exceed 100 distinct BSSIDs (measured: ~65 on 2.4+UNII-1
// alone). Too small a table fills before the sweep reaches UNII-3 and silently
// drops those APs. Sized generously, with keep-strongest eviction if it ever fills.
#define CR_MAX_APS 220
struct CrAP {
    char     ssid[24];
    uint8_t  bssid[6];
    int8_t   rssi;
    uint8_t  channel;    // primary / control channel
    uint8_t  centerCh;   // bonded center channel (where the energy sits)
    uint8_t  bwMhz;      // occupied bandwidth: 20 / 40 / 80 / 160
    uint8_t  phyGen;     // 0=b 1=g 2=n 3=ac 4=ax (highest PHY the AP advertises)
    uint8_t  enc;
    uint16_t color;
    uint32_t lastSeen;
};
static CrAP aps[CR_MAX_APS];
static int  apCount = 0;

static bool exitRequested = false;
static int  detailIndex   = -1;      // -1 = graph; else index into aps[]

static TFT_eSprite* spr = nullptr;

// ── Scan / hop state ─────────────────────────────────────────────────────────
// USA 2.4GHz hop sets, user-selectable on screen (SPEED/FULL button):
//   SPEED = 1/6/11 only — the only channels US routers actually park on, so this
//           is the fast default (adjacent-channel scans find nothing and never
//           affect a bell's width, which is drawn from fixed 20MHz bleed).
//   FULL  = every US 2.4GHz channel 1..11 — safety net for a stray hotspot/mesh
//           node on an off channel (3/4/8/9), slower sweep.
// 5GHz (US non-DFS) is appended in both modes; it must stay a subset of FIVE_G
// so every hopped channel has a plot position.
static const uint8_t CR_2G_SPEED[] = {1, 6, 11};
static const uint8_t CR_2G_FULL[]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static bool     fullScan = false;        // false = SPEED, true = FULL
static uint8_t  hopList[24];             // built from the 2.4 set + 5G set
static int      hopCount = 0;
static const int CR_DWELL_MS  = 155;    // passive dwell > 1 beacon interval (~100ms) so
                                        // 5GHz beacons aren't missed on a short dwell
static const uint32_t CR_STALE_MS = 8000;

static int      hopIdx      = 0;
static bool     scanEnabled = true;
static uint32_t nextHopAt   = 0;
static bool     bandInit    = false;
static bool     lastBand5   = false;

// refresh cadence (pause between full sweeps)
enum { RM_LIVE, RM_3S, RM_10S, RM_30S, RM_MANUAL, RM_COUNT };
static int refreshMode = RM_LIVE;
static const char* rmLabel(int m) {
    switch (m) { case RM_LIVE: return "LIVE"; case RM_3S: return "3s";
                 case RM_10S: return "10s";  case RM_30S: return "30s";
                 default: return "MAN"; }
}
static uint32_t rmPauseMs(int m) {
    switch (m) { case RM_3S: return 3000; case RM_10S: return 10000;
                 case RM_30S: return 30000; default: return 0; }
}

// ── Geometry ─────────────────────────────────────────────────────────────────
static const int GX = 0;
static const int GY = SCALE_Y(60);   // content top: below tab bar (row1) + control strip (row2)
static const int GW = SCREEN_WIDTH;
static const int CARD_H = 64;   // marker info-card height (sprite-local, bottom strip)
static int GH = 0;
static int plotL, plotR;

struct Band {
    bool        fiveG;
    const char* tag;
    int         rTop, rBottom;
    int         plotT, plotB;
};
static Band bandA, bandB;    // A = 2.4GHz (top), B = 5GHz (bottom)

static const uint16_t CR_PALETTE[] = {
    0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F,
    0xFD20, 0x87F0, 0xFC9F, 0xAFE5, 0x051F, 0xFDA0
};
static const int CR_PAL_N = sizeof(CR_PALETTE) / sizeof(CR_PALETTE[0]);

// USA 2.4GHz axis spans channels 1..11.
static const int TWOG_CH_N = 11;
// 5GHz channels drawn on the axis (matches the hopped 5G set).
static const uint8_t FIVE_G[] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
static const int     FIVE_G_N = sizeof(FIVE_G) / sizeof(FIVE_G[0]);

static WhRect detPrev, detNext, detClose, detAttack, detX;

// ── Tabbed shell: LIST (scanner) · SPECTRUM (plot) · WiFi6 (stub) ─────────────
enum { TAB_LIST, TAB_SPECTRUM, TAB_WIFI6, TAB_COUNT };
static int  activeTab = TAB_SPECTRUM;
static const int STRIP_Y = SCALE_Y(38);
static const int STRIP_H = SCALE_Y(21);
static WhRect stripBtn[4];
static int    stripN = 0;

// LIST view state — the list is a STATIC snapshot (never repaints mid-sweep so
// you don't chase a moving row); it redraws only on entry, scroll, or REFRESH.
static int  listScroll = 0;
static int  sortOrder[CR_MAX_APS];
static WhRect listUp, listDown;
static bool listNeedsRefresh = false;

// Settings/gear overlay (holds Refresh + Scan mode + Rescan, with explanations)
static bool settingsOpen = false;
static WhRect setRefresh, setScan, setClose;

// Attack target handoff (mirrors WifiScan: flags + selected*, .ino launches attack)
static bool popupOpen = false;
static char selBssidStr[18] = "";
static char selSsidStr[33]  = "";
static int  selChannel      = 0;
static bool deauthReq = false, cloneReq = false;
static WhRect popDeauth, popClone, popCancel;

// ─────────────────────────────────────────────────────────────────────────────
static const char* encStr(uint8_t e) {
    switch (e) {
        case WIFI_AUTH_OPEN:            return "OPEN";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        default:                        return "?";
    }
}

static bool apIsFiveG(const CrAP& a) { return wh_wifi_is_5g_channel(a.channel); }

// Map a (possibly fractional) channel number to a sprite X. The domain is padded
// ~2 channels beyond the first/last channel on each side so the ch1 / ch165 bells
// (which spill below ch1 / past the edge in frequency) sit INBOARD of the axes and
// draw in full instead of getting clipped on the y-axis.
static const float CR_PAD_CH = 2.2f;   // 2.4GHz padding, in channels
static const float CR_PAD_IX = 1.2f;   // 5GHz padding, in channel-index steps

static float chToX(float ch, bool fiveG) {
    if (!fiveG) {
        const float lo = 1.0f - CR_PAD_CH;
        const float hi = (float)TWOG_CH_N + CR_PAD_CH;
        return plotL + (ch - lo) / (hi - lo) * (plotR - plotL);
    }
    // 5GHz: convert channel -> fractional index within FIVE_G[], then pad. The
    // 48->149 DFS gap stays compressed to one index step (we don't scan it).
    float idx;
    if (ch <= FIVE_G[0]) {
        idx = (ch - FIVE_G[0]) / 4.0f;                       // extrapolate below 36
    } else if (ch >= FIVE_G[FIVE_G_N - 1]) {
        idx = (FIVE_G_N - 1) + (ch - FIVE_G[FIVE_G_N - 1]) / 4.0f;
    } else {
        idx = FIVE_G_N - 1;
        for (int i = 0; i < FIVE_G_N - 1; i++) {
            if (ch >= FIVE_G[i] && ch <= FIVE_G[i + 1]) {
                idx = (float)i + (ch - FIVE_G[i]) / (float)(FIVE_G[i + 1] - FIVE_G[i]);
                break;
            }
        }
    }
    const float lo = -CR_PAD_IX;
    const float hi = (float)(FIVE_G_N - 1) + CR_PAD_IX;
    return plotL + (idx - lo) / (hi - lo) * (plotR - plotL);
}

static int rssiToY(int rssi, int pt, int pb) {
    if (rssi > -30) rssi = -30;
    if (rssi < -95) rssi = -95;
    return pt + (int)((float)(rssi + 30) * (pb - pt) / (float)(-65));
}

// Bell center = the AP's bonded center channel (where the RF energy sits).
static float apCenterX(const CrAP& a) {
    bool    five = wh_wifi_is_5g_channel(a.channel);
    uint8_t c    = a.centerCh ? a.centerCh : a.channel;
    return chToX((float)c, five);
}

// Bell half-width in pixels = occupied bandwidth (20/40/80/160MHz), widened a
// little for stronger signals whose spectral skirts clear the noise floor.
static float apHalfPx(const CrAP& a) {
    bool    five  = wh_wifi_is_5g_channel(a.channel);
    uint8_t c     = a.centerCh ? a.centerCh : a.channel;
    float   halfCh = (a.bwMhz ? a.bwMhz : 20) / 10.0f;   // 20->2 .. 160->16 channel-nums
    float   sf     = ((float)a.rssi + 95.0f) / 65.0f;    // 0 weak .. 1 strong
    if (sf < 0.0f) sf = 0.0f;
    if (sf > 1.0f) sf = 1.0f;
    halfCh *= 0.85f + 0.30f * sf;   // width tracks bandwidth; strength only nudges it
    float xL = chToX((float)c - halfCh, five);
    float xR = chToX((float)c + halfCh, five);
    float h  = (xR - xL) * 0.5f;
    return h < 3.0f ? 3.0f : h;
}

// ── 802.11 PHY generation (from the scan record's phy_* flags) ───────────────
static uint8_t phyGenOf(const wifi_ap_record_t& r) {
    if (r.phy_11ax) return 4;   // Wi-Fi 6
    if (r.phy_11ac) return 3;   // Wi-Fi 5
    if (r.phy_11n)  return 2;   // Wi-Fi 4
    if (r.phy_11g)  return 1;
    return 0;                   // b
}
static const char* phyGenLabel(uint8_t g) {
    switch (g) { case 4: return "AX"; case 3: return "AC"; case 2: return "N"; case 1: return "G"; default: return "B"; }
}
static const char* phyGenLong(uint8_t g) {
    switch (g) { case 4: return "802.11ax  (Wi-Fi 6)"; case 3: return "802.11ac  (Wi-Fi 5)";
                 case 2: return "802.11n   (Wi-Fi 4)"; case 1: return "802.11g"; default: return "802.11b"; }
}
static uint16_t phyGenColor(uint8_t g) {
    switch (g) { case 4: return WONTHOUND_GREEN; case 3: return WONTHOUND_CYAN;
                 case 2: return WONTHOUND_MAGENTA; default: return WONTHOUND_GUNMETAL; }
}

// ── AP table maintenance ─────────────────────────────────────────────────────
// Derive occupied bandwidth + bonded center channel from a scan record.
static void apDeriveWidth(const wifi_ap_record_t& r, uint8_t& centerCh, uint8_t& bwMhz) {
    bool five = wh_wifi_is_5g_channel(r.primary);
    switch (r.bandwidth) {
        case WIFI_BW40:      bwMhz = 40;  break;
        case WIFI_BW80:      bwMhz = 80;  break;
        case WIFI_BW160:     bwMhz = 160; break;
        case WIFI_BW80_BW80: bwMhz = 80;  break;
        default:             bwMhz = 20;  break;   // WIFI_BW20 / HT20
    }
    // 2.4GHz physically tops out at 40MHz — ignore bogus 80/160 reports (some
    // 802.11ax APs advertise them) that would otherwise smear a huge bell.
    if (!five && bwMhz > 40) bwMhz = 40;

    uint8_t center = r.primary;
    if (five && (bwMhz == 80 || bwMhz == 160) && r.vht_ch_freq1 != 0) {
        center = r.vht_ch_freq1;                    // 5GHz 80/160 bonded center
    } else if (bwMhz == 40) {
        if      (r.second == WIFI_SECOND_CHAN_ABOVE) center = r.primary + 2;
        else if (r.second == WIFI_SECOND_CHAN_BELOW) center = (r.primary > 2) ? r.primary - 2 : r.primary;
    }
    centerCh = center;
}

static void upsertAp(const wifi_ap_record_t& r, uint32_t now) {
    for (int i = 0; i < apCount; i++) {
        if (memcmp(aps[i].bssid, r.bssid, 6) == 0) {
            aps[i].rssi     = r.rssi;
            aps[i].channel  = r.primary;
            aps[i].enc      = r.authmode;
            aps[i].phyGen   = phyGenOf(r);
            aps[i].lastSeen = now;
            apDeriveWidth(r, aps[i].centerCh, aps[i].bwMhz);
            return;
        }
    }
    int slot;
    if (apCount < CR_MAX_APS) {
        slot = apCount++;
    } else {
        // table full: replace the weakest AP, but only if this one is stronger
        int weakest = 0;
        for (int i = 1; i < apCount; i++)
            if (aps[i].rssi < aps[weakest].rssi) weakest = i;
        if (r.rssi <= aps[weakest].rssi) return;
        slot = weakest;
    }
    CrAP& a = aps[slot];
    memcpy(a.bssid, r.bssid, 6);
    const char* s = (const char*)r.ssid;
    if (s[0] == 0) snprintf(a.ssid, sizeof(a.ssid), "(hidden)");
    else           snprintf(a.ssid, sizeof(a.ssid), "%s", s);
    a.rssi     = r.rssi;
    a.channel  = r.primary;
    a.enc      = r.authmode;
    a.phyGen   = phyGenOf(r);
    a.lastSeen = now;
    a.color    = CR_PALETTE[(r.bssid[5] ^ r.bssid[4] ^ r.bssid[3] ^ r.bssid[2]) % CR_PAL_N];
    apDeriveWidth(r, a.centerCh, a.bwMhz);
}

static void ageOut(uint32_t now) {
    int w = 0;
    for (int i = 0; i < apCount; i++) {
        if (now - aps[i].lastSeen <= CR_STALE_MS) {
            if (w != i) aps[w] = aps[i];
            w++;
        }
    }
    apCount = w;
}

// rebuild the hop list from the current SPEED/FULL selection
static void buildHopList() {
    hopCount = 0;
    const uint8_t* g2 = fullScan ? CR_2G_FULL : CR_2G_SPEED;
    int n2 = fullScan ? (int)sizeof(CR_2G_FULL) : (int)sizeof(CR_2G_SPEED);
    for (int i = 0; i < n2 && hopCount < (int)sizeof(hopList); i++)         hopList[hopCount++] = g2[i];
    for (int i = 0; i < FIVE_G_N && hopCount < (int)sizeof(hopList); i++)   hopList[hopCount++] = FIVE_G[i];
    if (hopIdx >= hopCount) hopIdx = 0;
}

// ACTIVELY scan ONE channel (sends a probe request like the working WiFi Scanner
// — passive/beacon-only was missing 5GHz APs), merge results.
static void scanChannel(uint8_t ch) {
    bool is5 = wh_wifi_is_5g_channel(ch);
    if (!bandInit || is5 != lastBand5) {
        wh_wifi_prepare_channel(ch);   // switch band mode only on band change
        lastBand5 = is5;
        bandInit  = true;
        delay(35);                     // let the single radio retune to the new band
    }

    wifi_scan_config_t cfg = {};
    cfg.ssid          = nullptr;
    cfg.bssid         = nullptr;
    cfg.channel       = ch;
    cfg.show_hidden   = true;
    cfg.scan_type     = WIFI_SCAN_TYPE_ACTIVE;
    cfg.scan_time.active.min = 40;
    cfg.scan_time.active.max = 100;   // active scans respond fast; keep touch snappy

    esp_err_t serr = esp_wifi_scan_start(&cfg, true);
    uint16_t found = 0;
    if (serr == ESP_OK) esp_wifi_scan_get_ap_num(&found);
#ifdef CR_SELFTEST
    Serial.printf("[CR] ch%u band%dG start=0x%x found=%u\n", ch, is5 ? 5 : 2, serr, found);
#endif
    if (serr != ESP_OK || found == 0) return;

    // Grab up to 48 records per channel (dense channels return 30+; 24 was
    // truncating them — part of why UNII-3 looked empty).
    static wifi_ap_record_t recs[48];
    uint16_t n = found > 48 ? 48 : found;
    if (esp_wifi_scan_get_ap_records(&n, recs) != ESP_OK) return;

    uint32_t now = millis();
    for (int i = 0; i < n; i++) {
#ifdef CR_SELFTEST
        Serial.printf("   -> pri=%u bw=%u f1=%u rssi=%d ssid=%.16s\n",
                      recs[i].primary, recs[i].bandwidth, recs[i].vht_ch_freq1,
                      recs[i].rssi, (const char*)recs[i].ssid);
#endif
        upsertAp(recs[i], now);
    }
}

// ── Rendering ─────────────────────────────────────────────────────────────────
static void drawBandGrid(const Band& bd) {
    int cnt = 0;
    for (int i = 0; i < apCount; i++)
        if (apIsFiveG(aps[i]) == bd.fiveG) cnt++;

    spr->fillRoundRect(2, bd.rTop + 1, 40, 12, 3, WONTHOUND_DARK);
    spr->drawRoundRect(2, bd.rTop + 1, 40, 12, 3, WONTHOUND_HOTPINK);
    spr->setTextColor(WONTHOUND_HOTPINK);
    spr->setTextDatum(ML_DATUM);
    spr->setTextFont(1);
    spr->drawString(bd.tag, 5, bd.rTop + 7);

    // AP count RIGHT NEXT TO the band tag, in bright text (was a tiny dark number
    // in the top-right corner that was unreadable on black).
    spr->setTextColor(WONTHOUND_BRIGHT);
    spr->setTextDatum(ML_DATUM);
    char c[16];
    snprintf(c, sizeof(c), "%d APs", cnt);
    spr->drawString(c, 48, bd.rTop + 7);

    for (int db = -50; db >= -90; db -= 20) {
        int y = rssiToY(db, bd.plotT, bd.plotB);
        for (int x = plotL; x <= plotR; x += 5) spr->drawPixel(x, y, 0x2104);
        spr->setTextColor(WONTHOUND_GUNMETAL);
        spr->setTextDatum(MR_DATUM);
        char b[6];
        snprintf(b, sizeof(b), "%d", db);
        spr->drawString(b, plotL - 2, y);
    }

    spr->setTextDatum(MC_DATUM);
    if (!bd.fiveG) {
        for (int ch = 1; ch <= TWOG_CH_N; ch++) {
            int x = (int)chToX(ch, false);
            spr->drawFastVLine(x, bd.plotT, bd.plotB - bd.plotT, 0x1082);
            spr->setTextColor(WONTHOUND_CYAN);
            char b[4];
            snprintf(b, sizeof(b), "%d", ch);
            spr->drawString(b, x, bd.plotB + 6);
        }
    } else {
        for (int i = 0; i < FIVE_G_N; i++) {
            int x = (int)chToX(FIVE_G[i], true);
            spr->drawFastVLine(x, bd.plotT, bd.plotB - bd.plotT, 0x1082);
            spr->setTextColor(WONTHOUND_CYAN);
            char b[4];
            snprintf(b, sizeof(b), "%d", FIVE_G[i]);
            spr->drawString(b, x, bd.plotB + 6);
        }
    }

    spr->drawFastHLine(plotL, bd.plotB, plotR - plotL, WONTHOUND_VIOLET);
    spr->drawFastVLine(plotL, bd.plotT, bd.plotB - bd.plotT, WONTHOUND_VIOLET);
}

// draw one translucent bell for an AP into this band's plot area
static void drawOneBell(const CrAP& a, const Band& bd, uint8_t alpha, bool outlineBright) {
    int   peakX = (int)apCenterX(a);
    int   peakY = rssiToY(a.rssi, bd.plotT, bd.plotB);
    float half  = apHalfPx(a);
    uint16_t col     = a.color;
    uint16_t outline = outlineBright ? WONTHOUND_BRIGHT : col;
    int x0 = peakX - (int)half, x1 = peakX + (int)half;
    for (int x = x0; x <= x1; x++) {
        if (x < plotL || x > plotR) continue;
        float t = (float)(x - peakX) / half;
        float h = cosf(t * 1.5707963f);
        if (h <= 0.02f) continue;
        int cy = bd.plotB - (int)((bd.plotB - peakY) * h);
        if (cy < bd.plotT) cy = bd.plotT;
        for (int y = cy + 1; y < bd.plotB; y++) {
            uint16_t bg = spr->readPixel(x, y);
            spr->drawPixel(x, y, spr->alphaBlend(alpha, col, bg));
        }
        spr->drawPixel(x, cy, outline);
        if (cy + 1 < bd.plotB) spr->drawPixel(x, cy + 1, outline);
    }
}

static void drawBandBells(const Band& bd) {
    // every AP: translucent bell, width = bandwidth x strength, height = strength
    for (int i = 0; i < apCount; i++) {
        if (apIsFiveG(aps[i]) != bd.fiveG) continue;
        if (i == detailIndex) continue;                 // selected drawn last, on top
        drawOneBell(aps[i], bd, 64, false);
    }

    // selected AP (marker): brighter fill + bright outline + center line + label
    if (detailIndex >= 0 && detailIndex < apCount &&
        apIsFiveG(aps[detailIndex]) == bd.fiveG) {
        const CrAP& a = aps[detailIndex];
        int mx = (int)apCenterX(a);
        int py = rssiToY(a.rssi, bd.plotT, bd.plotB);
        for (int y = bd.plotT; y < bd.plotB; y += 2) spr->drawPixel(mx, y, WONTHOUND_BRIGHT);
        drawOneBell(a, bd, 120, true);
        spr->fillCircle(mx, py, 2, WONTHOUND_BRIGHT);
        spr->setTextDatum(BC_DATUM);
        spr->setTextFont(1);
        spr->setTextColor(WONTHOUND_BRIGHT);
        int ly = py - 3;
        if (ly < bd.plotT + 6) ly = bd.plotT + 6;
        spr->drawString(a.ssid, mx, ly);
    }
}

// compact marker card (top strip of the sprite) for the selected AP; keeps the
// plots visible so you can see the marker while stepping PREV/NEXT.
static void drawInfoCard() {
    const CrAP& a = aps[detailIndex];
    // Adaptive: put the card on the OPPOSITE half from the selected AP so it never
    // covers the bell you tapped — 5GHz AP (bottom plot) → card on TOP, 2.4 → bottom.
    int cy = apIsFiveG(a) ? 2 : (GH - CARD_H - 2);
    int cx = 2, cw = GW - 4, ch = CARD_H;
    spr->fillRoundRect(cx, cy, cw, ch, 5, WONTHOUND_DARK);
    spr->drawRoundRect(cx, cy, cw, ch, 5, a.color);

    spr->setTextFont(1);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(WONTHOUND_BRIGHT);
    spr->drawString(a.ssid, cx + 5, cy + 3);
    spr->setTextDatum(TR_DATUM);
    spr->setTextColor(a.color);
    char rs[12];
    snprintf(rs, sizeof(rs), "%d dBm", a.rssi);
    spr->drawString(rs, cx + cw - 5, cy + 3);

    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(WONTHOUND_MAGENTA);
    char l2[52];
    snprintf(l2, sizeof(l2), "ch%d/%dM %s  %02X:%02X:%02X:%02X:%02X:%02X",
             a.channel, a.bwMhz ? a.bwMhz : 20, encStr(a.enc),
             a.bssid[0], a.bssid[1], a.bssid[2], a.bssid[3], a.bssid[4], a.bssid[5]);
    spr->drawString(l2, cx + 5, cy + 14);

    // Three wide, well-separated buttons (10px gaps so a fat-finger tap can't
    // clip the neighbour) at the very bottom of the card.
    int gap = 10, bh = 30, by = cy + ch - bh - 3;
    int bw = (cw - 2 * gap - 4) / 3, bx0 = cx + 2;
    int xa = bx0, xb = bx0 + bw + gap, xc = bx0 + 2 * (bw + gap);
    spr->setTextDatum(MC_DATUM);
    spr->fillRoundRect(xa, by, bw, bh, 4, WONTHOUND_BLACK);
    spr->drawRoundRect(xa, by, bw, bh, 4, WONTHOUND_MAGENTA);
    spr->setTextColor(WONTHOUND_MAGENTA);
    spr->drawString("< PREV", xa + bw / 2, by + bh / 2);
    spr->fillRoundRect(xb, by, bw, bh, 4, WONTHOUND_DARK);
    spr->drawRoundRect(xb, by, bw, bh, 4, WONTHOUND_HOTPINK);
    spr->setTextColor(WONTHOUND_HOTPINK);
    spr->drawString("ATTACK", xb + bw / 2, by + bh / 2);
    spr->fillRoundRect(xc, by, bw, bh, 4, WONTHOUND_BLACK);
    spr->drawRoundRect(xc, by, bw, bh, 4, WONTHOUND_MAGENTA);
    spr->setTextColor(WONTHOUND_MAGENTA);
    spr->drawString("NEXT >", xc + bw / 2, by + bh / 2);
    spr->setTextDatum(TL_DATUM);

    // screen-coord hit rects (sprite is pushed at y = GY)
    detPrev   = {(int16_t)xa, (int16_t)(GY + by), (int16_t)bw, (int16_t)bh};
    detAttack = {(int16_t)xb, (int16_t)(GY + by), (int16_t)bw, (int16_t)bh};
    detNext   = {(int16_t)xc, (int16_t)(GY + by), (int16_t)bw, (int16_t)bh};
}

static void renderGraph() {
    if (!spr) return;
    spr->fillSprite(WONTHOUND_BLACK);
    spr->setTextSize(1);
    spr->setTextFont(1);

    drawBandGrid(bandA);
    drawBandBells(bandA);
    drawBandGrid(bandB);
    drawBandBells(bandB);

    int midY = (bandA.rBottom + bandB.rTop) / 2;
    for (int x = 0; x < GW; x += 4) spr->drawPixel(x, midY, WONTHOUND_VIOLET);

    if (detailIndex >= 0 && detailIndex < apCount) drawInfoCard();

    spr->setTextDatum(TL_DATUM);
    spr->pushSprite(GX, GY);
}

// ── Tab bar (row 1: Back + LIST/SPECTRUM/WiFi6) ──────────────────────────────
static const char* tabName(int t) {
    return t == TAB_LIST ? "LIST" : t == TAB_SPECTRUM ? "SPECTRUM" : "WiFi6";
}
static void drawTabBar() {
    whDrawBackButton();
    WhRect t[3];
    whTopBarActions(3, t);
    for (int i = 0; i < 3; i++)
        whTopBtn(t[i], tabName(i), i == activeTab ? WH_ON : WH_OUTLINE);
}

// ── Control strip (row 2: per-tab controls) ──────────────────────────────────
static void stripLayout(int n) {
    stripN = n;
    if (n <= 0) return;
    int gap = 4, total = GW - 4, w = (total - (n - 1) * gap) / n, x = 2;
    for (int i = 0; i < n; i++) {
        stripBtn[i] = {(int16_t)x, (int16_t)(STRIP_Y + 1), (int16_t)w, (int16_t)(STRIP_H - 2)};
        x += w + gap;
    }
}
static void drawControlStrip() {
    tft.fillRect(0, STRIP_Y, GW, STRIP_H, WONTHOUND_BLACK);
    if (activeTab != TAB_SPECTRUM) {
        // LIST + WiFi6 are static snapshots, so they get an explicit REFRESH.
        stripLayout(2);
        whTopBtn(stripBtn[0], "REFRESH", WH_ACCENT);
        whTopBtn(stripBtn[1], "SETTINGS", WH_OUTLINE);
        return;
    }
    // SPECTRUM: RESCAN only in MANUAL, otherwise show the live mode.
    if (refreshMode == RM_MANUAL) {
        stripLayout(2);
        whTopBtn(stripBtn[0], "RESCAN", WH_ACCENT);
        whTopBtn(stripBtn[1], "SETTINGS", WH_OUTLINE);
    } else {
        stripN = 1;
        int gw = 92;
        stripBtn[0] = {(int16_t)(GW - gw - 2), (int16_t)(STRIP_Y + 1), (int16_t)gw, (int16_t)(STRIP_H - 2)};
        tft.setTextFont(1); tft.setTextDatum(ML_DATUM); tft.setTextColor(WONTHOUND_GREEN);
        char s[28]; snprintf(s, sizeof(s), "%s  %s", rmLabel(refreshMode), fullScan ? "FULL" : "SPEED");
        tft.drawString(s, 5, STRIP_Y + STRIP_H / 2);
        tft.setTextDatum(TL_DATUM);
        whTopBtn(stripBtn[0], "SETTINGS", WH_OUTLINE);
    }
}

// ── Settings/gear overlay: Refresh + Scan + Rescan Now, each explained ────────
static void drawSettings() {
    int pw = GW - 24, ph = 166, px = 12, py = GY + 6;
    tft.fillRoundRect(px, py, pw, ph, 8, WONTHOUND_DARK);
    tft.drawRoundRect(px, py, pw, ph, 8, WONTHOUND_HOTPINK);
    tft.setTextFont(1); tft.setTextDatum(TC_DATUM);
    tft.setTextColor(WONTHOUND_HOTPINK);
    tft.drawString("SETTINGS", px + pw / 2, py + 7);
    tft.setTextDatum(TL_DATUM);

    int bx = px + 10, bw = pw - 20;
    // Refresh mode (tap cycles)
    setRefresh = {(int16_t)bx, (int16_t)(py + 22), (int16_t)bw, 24};
    tft.fillRoundRect(bx, setRefresh.y, bw, 24, 4, WONTHOUND_BLACK);
    tft.drawRoundRect(bx, setRefresh.y, bw, 24, 4, WONTHOUND_MAGENTA);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(WONTHOUND_BRIGHT);
    char r[28]; snprintf(r, sizeof(r), "Refresh:  %s", rmLabel(refreshMode));
    tft.drawString(r, px + pw / 2, setRefresh.y + 12);
    tft.setTextDatum(TL_DATUM); tft.setTextColor(WONTHOUND_CYAN);
    tft.setCursor(bx, py + 49);  tft.print("LIVE=nonstop  3/10/30s=pause");
    tft.setCursor(bx, py + 59);  tft.print("MANUAL=only on Rescan Now");

    // Scan coverage (tap toggles)
    setScan = {(int16_t)bx, (int16_t)(py + 72), (int16_t)bw, 24};
    tft.fillRoundRect(bx, setScan.y, bw, 24, 4, WONTHOUND_BLACK);
    tft.drawRoundRect(bx, setScan.y, bw, 24, 4, WONTHOUND_MAGENTA);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(WONTHOUND_BRIGHT);
    char sc[30]; snprintf(sc, sizeof(sc), "Scan:  %s", fullScan ? "FULL (1-11)" : "SPEED (1/6/11)");
    tft.drawString(sc, px + pw / 2, setScan.y + 12);
    tft.setTextDatum(TL_DATUM); tft.setTextColor(WONTHOUND_CYAN);
    tft.setCursor(bx, py + 99);  tft.print("SPEED=fast, 2.4 on 1/6/11");
    tft.setCursor(bx, py + 109); tft.print("FULL=every 2.4 ch (thorough)");

    // Close  (RESCAN is NOT here — it lives on the strip, only in MANUAL mode)
    setClose = {(int16_t)bx, (int16_t)(py + 126), (int16_t)bw, 28};
    tft.fillRoundRect(bx, setClose.y, bw, 28, 4, WONTHOUND_BLACK);
    tft.drawRoundRect(bx, setClose.y, bw, 28, 4, WONTHOUND_HOTPINK);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(WONTHOUND_HOTPINK);
    tft.drawString("CLOSE", px + pw / 2, setClose.y + 14);
    tft.setTextDatum(TL_DATUM);
}

// ── LIST view (shares aps[] + detailIndex with the spectrum) ─────────────────
static void buildSortOrder() {
    for (int i = 0; i < apCount; i++) sortOrder[i] = i;
    for (int i = 1; i < apCount; i++) {           // insertion sort by RSSI desc
        int v = sortOrder[i], j = i - 1;
        while (j >= 0 && aps[sortOrder[j]].rssi < aps[v].rssi) { sortOrder[j + 1] = sortOrder[j]; j--; }
        sortOrder[j + 1] = v;
    }
}
static void drawSigBars(int x, int y, int rssi) {
    int bars = rssi >= -50 ? 4 : rssi >= -60 ? 3 : rssi >= -70 ? 2 : rssi >= -80 ? 1 : 0;
    uint16_t col = bars >= 3 ? WONTHOUND_GREEN : bars >= 2 ? 0xFFE0 : WONTHOUND_HOTPINK;
    for (int i = 0; i < 4; i++) {
        int h = 3 + i * 2;
        tft.fillRect(x + i * 4, y + (9 - h), 3, h, i < bars ? col : WONTHOUND_GUNMETAL);
    }
}
// Big, easy-to-read rows: one line each (SSID + channel + signal), fewer per page.
#define LIST_ROW_H   34
static const int LIST_BAR_H = 40;   // bottom scroll bar
static const int LIST_GAP   = 12;   // dead gap between last row and the scroll bar
static int listRowsTop()    { return GY; }
static int listRowsBottom() { return SCREEN_HEIGHT - LIST_BAR_H - LIST_GAP; }
static int listRows()       { return (listRowsBottom() - listRowsTop()) / LIST_ROW_H; }

static void drawList() {
    int top = listRowsTop(), rows = listRows();
    int barY = SCREEN_HEIGHT - LIST_BAR_H;
    tft.fillRect(0, GY, GW, SCREEN_HEIGHT - GY, WONTHOUND_BLACK);
    buildSortOrder();
    if (listScroll > apCount - rows) listScroll = apCount - rows;
    if (listScroll < 0) listScroll = 0;

    if (apCount == 0) {
        tft.setTextFont(2); tft.setTextDatum(MC_DATUM);
        tft.setTextColor(WONTHOUND_GUNMETAL);
        tft.drawString("Scanning airwaves...", GW / 2, top + 40);
        tft.setTextDatum(TL_DATUM);
    }
    for (int r = 0; r < rows; r++) {
        int idx = listScroll + r;
        if (idx >= apCount) break;
        int ap = sortOrder[idx];
        int y = top + r * LIST_ROW_H;
        bool sel = (ap == detailIndex);
        uint16_t bg = sel ? WONTHOUND_DARK : WONTHOUND_BLACK;
        tft.fillRect(0, y, GW, LIST_ROW_H - 2, bg);
        tft.fillRect(2, y + 5, 6, LIST_ROW_H - 12, aps[ap].color);   // color chip
        // SSID (big, left)
        tft.setTextFont(2); tft.setTextDatum(ML_DATUM);
        tft.setTextColor(sel ? WONTHOUND_BRIGHT : WONTHOUND_MAGENTA, bg);
        char nm[18]; snprintf(nm, sizeof(nm), "%.15s", aps[ap].ssid);
        tft.drawString(nm, 14, y + LIST_ROW_H / 2 - 1);
        // right column: generation badge on the WiFi6 tab, channel on LIST; + bars
        tft.setTextFont(1); tft.setTextDatum(MR_DATUM);
        if (activeTab == TAB_WIFI6) {
            tft.setTextColor(phyGenColor(aps[ap].phyGen), bg);
            tft.drawString(phyGenLabel(aps[ap].phyGen), GW - 30, y + LIST_ROW_H / 2 - 1);
        } else {
            tft.setTextColor(WONTHOUND_CYAN, bg);
            char chs[10]; snprintf(chs, sizeof(chs), "CH%d", aps[ap].channel);
            tft.drawString(chs, GW - 30, y + LIST_ROW_H / 2 - 1);
        }
        drawSigBars(GW - 24, y + LIST_ROW_H / 2 - 5, aps[ap].rssi);
        tft.setTextDatum(TL_DATUM);
    }

    // position readout in the gap, just above the (separated) scroll bar
    if (apCount > 0) {
        int last = listScroll + rows; if (last > apCount) last = apCount;
        char f[24]; snprintf(f, sizeof(f), "%d-%d of %d", listScroll + 1, last, apCount);
        tft.setTextFont(1); tft.setTextDatum(MC_DATUM);
        tft.setTextColor(WONTHOUND_VIOLET);
        tft.drawString(f, GW / 2, barY - LIST_GAP / 2 - 1);
        tft.setTextDatum(TL_DATUM);
    }
    // separated bottom scroll bar: two big buttons with a gap between them
    int gap = 6, half = (GW - 4 - gap) / 2;
    listUp   = {(int16_t)2,                 (int16_t)(barY + 2), (int16_t)half, (int16_t)(LIST_BAR_H - 4)};
    listDown = {(int16_t)(2 + half + gap),  (int16_t)(barY + 2), (int16_t)half, (int16_t)(LIST_BAR_H - 4)};
    whTopBtn(listUp,   "^  UP",   WH_ACCENT);
    whTopBtn(listDown, "DOWN  v", WH_ACCENT);
}

// ── Attack chooser popup (shared by LIST + SPECTRUM) ─────────────────────────
static void captureTarget() {
    if (detailIndex < 0 || detailIndex >= apCount) return;
    const CrAP& a = aps[detailIndex];
    snprintf(selSsidStr, sizeof(selSsidStr), "%s", a.ssid);
    snprintf(selBssidStr, sizeof(selBssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             a.bssid[0], a.bssid[1], a.bssid[2], a.bssid[3], a.bssid[4], a.bssid[5]);
    selChannel = a.channel;
}
// Full target detail + actions (opened by a LIST row tap or the SPECTRUM ATTACK button)
static void drawAttackPopup() {
    if (detailIndex < 0 || detailIndex >= apCount) { popupOpen = false; return; }
    const CrAP& a = aps[detailIndex];
    int px = 12, py = GY + 6, pw = GW - 24, ph = SCREEN_HEIGHT - py - 6;
    tft.fillRoundRect(px, py, pw, ph, 8, WONTHOUND_DARK);
    tft.drawRoundRect(px, py, pw, ph, 8, a.color);
    tft.drawRoundRect(px + 1, py + 1, pw - 2, ph - 2, 7, a.color);

    int lx = px + 12;
    // SSID (big)
    tft.setTextFont(2); tft.setTextSize(1); tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WONTHOUND_BRIGHT, WONTHOUND_DARK);
    char nm[22]; snprintf(nm, sizeof(nm), "%.18s", a.ssid);
    tft.setCursor(lx, py + 8); tft.print(nm);

    tft.setTextFont(1);
    char line[46];
    tft.setTextColor(WONTHOUND_MAGENTA, WONTHOUND_DARK);
    snprintf(line, sizeof(line), "BSSID  %02X:%02X:%02X:%02X:%02X:%02X",
             a.bssid[0], a.bssid[1], a.bssid[2], a.bssid[3], a.bssid[4], a.bssid[5]);
    tft.setCursor(lx, py + 34); tft.print(line);
    tft.setTextColor(WONTHOUND_CYAN, WONTHOUND_DARK);
    snprintf(line, sizeof(line), "CH %d    %d MHz    %s",
             a.channel, a.bwMhz ? a.bwMhz : 20, wh_wifi_band_label(a.channel));
    tft.setCursor(lx, py + 50); tft.print(line);
    tft.setTextColor(phyGenColor(a.phyGen), WONTHOUND_DARK);
    snprintf(line, sizeof(line), "Wi-Fi   %s", phyGenLong(a.phyGen));
    tft.setCursor(lx, py + 64); tft.print(line);
    tft.setTextColor(WONTHOUND_CYAN, WONTHOUND_DARK);
    snprintf(line, sizeof(line), "Security   %s", encStr(a.enc));
    tft.setCursor(lx, py + 78); tft.print(line);
    snprintf(line, sizeof(line), "Signal     %d dBm", a.rssi);
    tft.setCursor(lx, py + 92); tft.print(line);
    int barX = lx, barY = py + 106, barW = pw - 24, barH = 10;
    tft.drawRect(barX, barY, barW, barH, WONTHOUND_MAGENTA);
    int fill = map(constrain((int)a.rssi, -95, -30), -95, -30, 0, barW - 2);
    tft.fillRect(barX + 1, barY + 1, fill, barH - 2, a.color);

    // big, well-separated actions at the bottom
    int bw = pw - 24, bh = 34, gap = 12, bx = px + 12;
    int by = py + ph - 3 * bh - 2 * gap - 8;
    popDeauth = {(int16_t)bx, (int16_t)by,                    (int16_t)bw, (int16_t)bh};
    popClone  = {(int16_t)bx, (int16_t)(by + bh + gap),       (int16_t)bw, (int16_t)bh};
    popCancel = {(int16_t)bx, (int16_t)(by + 2 * (bh + gap)), (int16_t)bw, (int16_t)bh};
    tft.setTextDatum(MC_DATUM);
    tft.fillRoundRect(popDeauth.x, popDeauth.y, bw, bh, 5, WONTHOUND_BLACK);
    tft.drawRoundRect(popDeauth.x, popDeauth.y, bw, bh, 5, WONTHOUND_HOTPINK);
    tft.setTextColor(WONTHOUND_HOTPINK);
    tft.drawString("DEAUTH", px + pw / 2, popDeauth.y + bh / 2);
    tft.fillRoundRect(popClone.x, popClone.y, bw, bh, 5, WONTHOUND_BLACK);
    tft.drawRoundRect(popClone.x, popClone.y, bw, bh, 5, WONTHOUND_CYAN);
    tft.setTextColor(WONTHOUND_CYAN);
    tft.drawString("CLONE (Evil Twin)", px + pw / 2, popClone.y + bh / 2);
    tft.fillRoundRect(popCancel.x, popCancel.y, bw, bh, 5, WONTHOUND_BLACK);
    tft.drawRoundRect(popCancel.x, popCancel.y, bw, bh, 5, WONTHOUND_MAGENTA);
    tft.setTextColor(WONTHOUND_MAGENTA);
    tft.drawString("CLOSE", px + pw / 2, popCancel.y + bh / 2);
    tft.setTextDatum(TL_DATUM);
}

// ── active-tab render dispatch ───────────────────────────────────────────────
static void renderActiveTab() {
    if (activeTab == TAB_SPECTRUM) renderGraph();
    else                           drawList();   // LIST + WiFi6 are both list views
    if (settingsOpen) drawSettings();
    if (popupOpen)    drawAttackPopup();
}
static void switchTab(int t) {
    activeTab = t;
    drawTabBar();
    drawControlStrip();
    renderActiveTab();
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    exitRequested = false;
    detailIndex   = -1;
    apCount       = 0;
    hopIdx        = 0;
    scanEnabled   = true;
    nextHopAt     = 0;
    bandInit      = false;
    fullScan      = false;
    popupOpen     = false;
    settingsOpen  = false;
    deauthReq     = false;
    cloneReq      = false;
    listScroll    = 0;
    activeTab     = TAB_SPECTRUM;
    buildHopList();

    GH    = SCREEN_HEIGHT - GY - 2;
    plotL = 22;
    plotR = GW - 4;

    int mid = GH / 2;
    bandA = {false, "2.4G", 0,       mid - 3, 0, 0};
    bandB = {true,  "5GHz", mid + 3, GH - 1,  0, 0};
    bandA.plotT = bandA.rTop + 14;  bandA.plotB = bandA.rBottom - 11;
    bandB.plotT = bandB.rTop + 14;  bandB.plotB = bandB.rBottom - 11;

    tft.fillScreen(WONTHOUND_BLACK);

    if (!spr) {
        spr = new TFT_eSprite(&tft);
        spr->setColorDepth(16);
        spr->createSprite(GW, GH);
    }

    WiFi.mode(WIFI_STA);
    delay(60);
    wh_wifi_configure_dual_band();

    drawTabBar();
    drawControlStrip();
    renderActiveTab();   // empty grid; sweeps populate it
}

// ─────────────────────────────────────────────────────────────────────────────
// Self-running diagnostic: sweep every channel forever, dump what the radio
// returns per channel + the resulting AP table. Isolates scan-vs-render.
void selfTest() {
    Serial.println("\n[CR-TEST] ===== ChannelRadar scan self-test =====");
    apCount = 0; bandInit = false; fullScan = true; buildHopList();
    WiFi.mode(WIFI_STA);
    delay(150);
    wh_wifi_configure_dual_band();
    static const uint8_t testChans[] = {1, 6, 11, 36, 40, 44, 48, 149, 153, 157, 161, 165};
    int sweep = 0;
    while (true) {
        Serial.printf("[CR-TEST] ---------- sweep %d ----------\n", ++sweep);
        for (unsigned i = 0; i < sizeof(testChans); i++) scanChannel(testChans[i]);
        Serial.printf("[CR-TEST] AP TABLE (%d):\n", apCount);
        for (int i = 0; i < apCount; i++)
            Serial.printf("[CR-TEST]   ch=%u ctr=%u bw=%u rssi=%d 5g=%d  %.20s\n",
                          aps[i].channel, aps[i].centerCh, aps[i].bwMhz, aps[i].rssi,
                          (int)wh_wifi_is_5g_channel(aps[i].channel), aps[i].ssid);
        ageOut(millis());
        delay(1500);
    }
}

static int nearestInBand(bool fiveG, int localX) {
    int   best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < apCount; i++) {
        if (apIsFiveG(aps[i]) != fiveG) continue;
        float d = fabsf(apCenterX(aps[i]) - localX);
        if (d < bestD) { bestD = d; best = i; }
    }
    if (best >= 0 && bestD < apHalfPx(aps[best]) + 6.0f) return best;
    return -1;
}

void loop() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        // ── settings/gear overlay owns all touch while open ──
        if (settingsOpen) {
            waitForTouchRelease();
            if (whHit(tx, ty, setRefresh)) {
                refreshMode = (refreshMode + 1) % RM_COUNT;
                if (refreshMode != RM_MANUAL) { scanEnabled = true; nextHopAt = 0; }
                drawSettings();
            } else if (whHit(tx, ty, setScan)) {
                fullScan = !fullScan; buildHopList(); hopIdx = 0; scanEnabled = true; nextHopAt = 0;
                drawSettings();
            } else if (whHit(tx, ty, setClose) || whHit(tx, ty, whBackButtonRect())) {
                settingsOpen = false; drawControlStrip(); renderActiveTab();
            }
            return;
        }
        // ── attack popup owns all touch while open ──
        if (popupOpen) {
            waitForTouchRelease();
            if (whHit(tx, ty, popDeauth)) {
                captureTarget(); deauthReq = true; exitRequested = true; return;
            } else if (whHit(tx, ty, popClone)) {
                captureTarget(); cloneReq = true; exitRequested = true; return;
            } else if (whHit(tx, ty, popCancel) || whHit(tx, ty, whBackButtonRect())) {
                popupOpen = false; renderActiveTab();
            }
            return;
        }
        // ── spectrum marker card open ──
        if (activeTab == TAB_SPECTRUM && detailIndex >= 0) {
            waitForTouchRelease();
            if (whHit(tx, ty, whBackButtonRect())) {            // back = close card
                detailIndex = -1; renderGraph();
            } else if (whHit(tx, ty, detAttack)) {
                popupOpen = true; drawAttackPopup();
            } else if (whHit(tx, ty, detPrev)) {
                if (apCount > 0) { detailIndex = (detailIndex - 1 + apCount) % apCount; renderGraph(); }
            } else if (whHit(tx, ty, detNext)) {
                if (apCount > 0) { detailIndex = (detailIndex + 1) % apCount; renderGraph(); }
            } else {
                // tap outside the (adaptive) card → reselect nearest, or deselect
                int cardCy  = apIsFiveG(aps[detailIndex]) ? 2 : (GH - CARD_H - 2);
                int cardTop = GY + cardCy, cardBot = cardTop + CARD_H;
                if ((int)ty < cardTop || (int)ty >= cardBot) {
                    int localX = (int)tx - GX, localY = (int)ty - GY;
                    bool fiveG = (localY >= bandB.rTop);
                    int hit = nearestInBand(fiveG, localX);
                    if (hit < 0) hit = nearestInBand(!fiveG, localX);
                    detailIndex = hit;                          // -1 closes the card
                    renderGraph();
                }
            }
            return;
        }
        // ── tab bar (row 1) ──
        WhRect tb[3];
        whTopBarActions(3, tb);
        if (whHit(tx, ty, whBackButtonRect())) {
            waitForTouchRelease(); exitRequested = true; return;
        }
        for (int i = 0; i < 3; i++) {
            if (whHit(tx, ty, tb[i])) { waitForTouchRelease(); if (i != activeTab) switchTab(i); return; }
        }
        // ── control strip (row 2) ──
        if (ty >= STRIP_Y && ty < STRIP_Y + STRIP_H) {
            waitForTouchRelease();
            if (activeTab != TAB_SPECTRUM) {
                if (whHit(tx, ty, stripBtn[0])) {          // REFRESH: snapshot now + get fresh data
                    scanEnabled = true; hopIdx = 0; nextHopAt = 0; listNeedsRefresh = true; drawList();
                } else if (whHit(tx, ty, stripBtn[1])) { settingsOpen = true; drawSettings(); }
            } else if (refreshMode == RM_MANUAL) {
                if (whHit(tx, ty, stripBtn[0]))      { scanEnabled = true; hopIdx = 0; nextHopAt = 0; }
                else if (whHit(tx, ty, stripBtn[1])) { settingsOpen = true; drawSettings(); }
            } else {
                if (whHit(tx, ty, stripBtn[0]))      { settingsOpen = true; drawSettings(); }
            }
            return;
        }
        // ── content area ──
        if (ty >= GY) {
            waitForTouchRelease();
            if (activeTab == TAB_SPECTRUM) {
                int localX = (int)tx - GX, localY = (int)ty - GY;
                bool fiveG = (localY >= bandB.rTop);
                int hit = nearestInBand(fiveG, localX);
                if (hit < 0) hit = nearestInBand(!fiveG, localX);
                if (hit >= 0) { detailIndex = hit; renderGraph(); }
            } else {   // LIST + WiFi6
                int rows = listRows();
                int page = rows - 1; if (page < 1) page = 1;
                if (whHit(tx, ty, listUp))        { listScroll -= page; drawList(); }
                else if (whHit(tx, ty, listDown)) { listScroll += page; drawList(); }
                else if ((int)ty < listRowsBottom()) {        // only the rows area selects
                    int r = ((int)ty - GY) / LIST_ROW_H;
                    int idx = listScroll + r;
                    if (r >= 0 && r < rows && idx < apCount) {
                        detailIndex = sortOrder[idx];
                        drawList();
                        popupOpen = true; drawAttackPopup();
                    }
                }
            }
            return;
        }
        return;
    }

    // ── pause sweeping while an overlay or the spectrum marker card is open ──
    if (settingsOpen || popupOpen) { delay(20); return; }
    if (activeTab == TAB_SPECTRUM && detailIndex >= 0) { delay(20); return; }

    // ── one channel hop per tick; repaint the active tab on a full sweep ──
    if (scanEnabled && hopCount > 0 && (int32_t)(millis() - nextHopAt) >= 0) {
        scanChannel(hopList[hopIdx]);
        hopIdx++;
        if (hopIdx >= hopCount) {
            hopIdx = 0;
            ageOut(millis());
            // SPECTRUM repaints live; the LIST + WiFi6 list views only repaint on an
            // explicit REFRESH (listNeedsRefresh) so rows never move under your finger.
            if (activeTab != TAB_SPECTRUM) {
                if (listNeedsRefresh) { drawList(); listNeedsRefresh = false; }
            } else {
                renderActiveTab();
            }
            if (refreshMode == RM_MANUAL) scanEnabled = false;
            else                          nextHopAt = millis() + rmPauseMs(refreshMode);
        }
    }
    delay(5);
}

// ─────────────────────────────────────────────────────────────────────────────
bool isExitRequested() { return exitRequested; }

// ── attack handoff (mirrors WifiScan; .ino launches Deauther / CaptivePortal) ─
bool        isDeauthRequested()  { return deauthReq; }
bool        isCloneRequested()   { return cloneReq; }
const char* getSelectedBSSID()   { return selBssidStr; }
const char* getSelectedSSID()    { return selSsidStr; }
int         getSelectedChannel() { return selChannel; }
void        clearAttackRequest() { deauthReq = false; cloneReq = false; }

void cleanup() {
    esp_wifi_scan_stop();
    if (spr) {
        spr->deleteSprite();
        delete spr;
        spr = nullptr;
    }
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
}

}  // namespace ChannelRadar
