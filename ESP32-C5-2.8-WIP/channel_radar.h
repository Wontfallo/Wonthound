#ifndef CHANNEL_RADAR_H
#define CHANNEL_RADAR_H

// ═══════════════════════════════════════════════════════════════════════════
// WiFi Channel Occupancy ("Airwaves") — spectrum-analyzer style view of every
// AP as a translucent overlapping bell (arc) proportional to signal strength,
// positioned by channel with 2.4GHz overlap/bleed. Tap an arc for details,
// PREV/NEXT to scroll through targets, and toggle the 2.4G / 5G band.
// ═══════════════════════════════════════════════════════════════════════════

namespace ChannelRadar {
void setup();
void loop();
bool isExitRequested();
void cleanup();

// Self-running scan diagnostic (compiled in only with -DCR_SELFTEST): sweeps
// every channel forever and dumps per-channel results to Serial. Never returns.
void selfTest();

// Attack target handoff (LIST or SPECTRUM tap → DEAUTH / CLONE), consumed by the
// .ino dispatch which launches Deauther / CaptivePortal — same pattern as WifiScan.
bool        isDeauthRequested();
bool        isCloneRequested();
const char* getSelectedBSSID();
const char* getSelectedSSID();
int         getSelectedChannel();
void        clearAttackRequest();
}  // namespace ChannelRadar

#endif  // CHANNEL_RADAR_H
