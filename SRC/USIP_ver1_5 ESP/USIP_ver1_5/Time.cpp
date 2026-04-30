// Time.cpp
//
// Ported from the original time_update() function. Same control flow,
// state lives in g_state instead of file-scope globals. APRS-burst flags
// are set via Telemetry's public API.

#include "Time.h"
#include "Config.h"
#include "State.h"
#include "Telemetry.h"
#include <Arduino.h>

namespace {
  // Tracks "have we ever seen a GPS fix" so the first-sync path runs once.
  bool g_first_sync_done = false;
}

void Time_init() {
  g_state.next_sec_ms = millis() + 1000;
  g_first_sync_done   = false;
}

// Helper: schedule APRS metadata bursts at the canonical mission-clock
// instants (minutes 5/25/45, seconds 0/10/20/30).
static void scheduleBursts() {
  if ((g_state.mis_min % APRS_BURST_MINUTE_MOD) != APRS_BURST_MINUTE) return;
  switch (g_state.mis_sec) {
    case 0:  Telemetry_requestParm(); break;
    case 10: Telemetry_requestUnit(); break;
    case 20: Telemetry_requestEqu();  break;
    case 30: Telemetry_requestBit();  break;
    default: break;
  }
}

// Helper: advance mission clock by 1 second, never wrapping the hour
// (mission time accumulates beyond 24h; only minute and second wrap).
static void incMissionSecond() {
  g_state.mis_sec += 1;
  if (g_state.mis_sec >= 60) {
    g_state.mis_sec = 0;
    g_state.mis_min += 1;
    if (g_state.mis_min >= 60) {
      g_state.mis_min = 0;
      g_state.mis_hr  += 1;
    }
  }
}

void Time_tick() {
  // The 100 ms hysteresis matches the original `if (millis() >= next_sec + 100)`
  // at the top of the original loop — gives 100 ms of slack for serial work
  // before forcing an internal-clock advance.
  if ((int32_t)(millis() - g_state.next_sec_ms) < 100) return;

  if (g_state.time_sync == TS_NEVER) {
    // No GPS yet — advance internal clock.
    g_state.next_sec_ms += 1000;
    incMissionSecond();
    return;
  }

  // Time-sync is live and GPS did NOT arrive this second. Advance both
  // GPS clock (best-effort) and mission clock manually.
  if (millis() - g_state.last_gps_ms > GPS_TIMEOUT_MS) g_state.gps = GPS_INVALID;

  g_state.gps_sec += 1;
  if (g_state.gps_sec == 60) {
    g_state.gps_sec = 0;
    g_state.gps_min += 1;
    if (g_state.gps_min == 60) {
      g_state.gps_min = 0;
      g_state.gps_hr  += 1;
      if (g_state.gps_hr == 24) g_state.gps_hr = 0;
    }
  }
  incMissionSecond();
  g_state.next_sec_ms += 1000;

  scheduleBursts();
}

void Time_onGpsFix(int gps_hr, int gps_min, int gps_sec, uint32_t at_ms) {
  g_state.gps_hr  = gps_hr;
  g_state.gps_min = gps_min;
  g_state.gps_sec = gps_sec;
  g_state.last_gps_ms = at_ms;

  if (!g_first_sync_done) {
    // First valid GPS — initial alignment.
    g_first_sync_done       = true;
    g_state.time_sync       = TS_LIVE;
    g_state.time_sync_at_ms = at_ms;
    g_state.mis_sec         = gps_sec;
    g_state.next_sec_ms     = at_ms + 1000;
    Serial.print("Time Sync to OWN_GPS at ");
    Serial.print(at_ms / 1000);
    Serial.println(" seconds.");
    // Arbitrarily advance the mission clock to the next minute.
    g_state.mis_min += 1;
    if (g_state.mis_min >= 60) {
      g_state.mis_min = 0;
      g_state.mis_hr  += 1;
    }
    // Initialize alt history so EXPECT computation has a baseline.
    g_state.gps_alt1 = g_state.gps_alt;
    g_state.gps_alt2 = g_state.gps_alt;
    g_state.gps_alt3 = g_state.gps_alt;
    return;
  }

  // Subsequent GPS sync — adjust mission minute if internal clock has
  // drifted across the minute boundary relative to GPS.
  if (g_state.mis_sec < 10 && gps_sec > 50) {
    g_state.mis_min -= 1;
    if (g_state.mis_min == -1) {
      g_state.mis_min = 59;
      g_state.mis_hr  -= 1;
      if (g_state.mis_hr < 0) g_state.mis_hr = 0;
    }
  } else if (g_state.mis_sec > 50 && gps_sec < 10) {
    g_state.mis_min += 1;
    if (g_state.mis_min >= 60) {
      g_state.mis_min = 0;
      g_state.mis_hr  += 1;
    }
  }

  g_state.mis_sec     = gps_sec;
  g_state.next_sec_ms = at_ms + 1000;
  scheduleBursts();
}
