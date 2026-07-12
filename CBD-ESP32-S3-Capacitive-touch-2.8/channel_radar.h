#ifndef CHANNEL_RADAR_H
#define CHANNEL_RADAR_H

// WiFi Channel Occupancy ("Airwaves") - ESP32-S3 2.4GHz spectrum/list view.
// APs are drawn as overlapping signal bells by channel. Tap an arc or list row
// for details and Deauth / Clone handoff. S3 hardware is 2.4GHz-only here:
// no 5GHz scan path and no WiFi6 tab.

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
