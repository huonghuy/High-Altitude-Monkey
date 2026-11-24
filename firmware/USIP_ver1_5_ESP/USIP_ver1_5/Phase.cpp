// Phase.cpp
#include "Phase.h"
#include "Config.h"
#include "State.h"
#include <Arduino.h>

namespace {
  // Pressure-altitude history (used for vspeed estimate when GPS is stale).
  float g_pa[4] = { 0, 0, 0, 0 };
  float g_pa_vspeed = 0.0f;
  float g_vspeed_avg = 0.0f;

  // VspeedAvg thresholds (feet/sec).
  constexpr float VSPEED_PRELAUNCH_BAND   = 2.0f;    // |v| < this → prelaunch
  constexpr float VSPEED_RISING_THRESH    = 10.0f;
  constexpr float VSPEED_FALLING_THRESH   = -10.0f;
  constexpr float VSPEED_LANDED_BAND      = 4.0f;
  constexpr float ALT_RISING_STEP_FT      = 40.0f;   // 40-ft growth per 15s in prelaunch → rising
  constexpr float ALT_CAL_VSPEED_BAND     = 1.0f;
  constexpr uint32_t GPS_STALE_FOR_PA_MS  = 21000;

  // Abort debounce: 12-sample rolling window of "is gps_alt > max_alt*1000".
  bool     g_over_threshold[ABORT_DEBOUNCE_COUNT] = { false };
  uint8_t  g_over_idx = 0;
  uint8_t  g_over_filled = 0;   // how many samples we've actually written
}

void Phase_handleAbort(AbortReason reason) {
  // `aborting` is parallel to the natural phase — keep PHASE_RISING etc.
  // intact so the pressure-altitude calibrations and phase telemetry bits
  // continue to evolve correctly underneath the abort.
  g_state.aborting          = true;
  g_state.end_abort_ms      = millis() + ENDABORT_PULSE_MS;
  g_state.abort_pin_pulsing = true;

  const char* msg = nullptr;
  switch (reason) {
    case ABORT_LOCAL_CMD:    msg = "ABORTING - Local Command!!!";       break;
    case ABORT_RADIO_CMD:    msg = "ABORTING - Remote Command !!!";     break;
    case ABORT_GPS_MAX_ALT:  msg = "ABORTING - above GPS Max Altitude !!!"; break;
  }
  if (msg) {
    Serial.println(msg);
    Serial1.println(msg);
  }
}

void Phase_pinTick() {
  if (g_state.abort_pin_pulsing) {
    digitalWrite(PIN_ABORT, LOW);
    if ((int32_t)(millis() - g_state.end_abort_ms) >= 0) {
      digitalWrite(PIN_ABORT, HIGH);
      g_state.abort_pin_pulsing = false;
      // Note: `g_state.aborting` stays true until the phase tick clears it
      // (descended below max_alt). Same semantics as the original code.
    }
  } else {
    digitalWrite(PIN_ABORT, HIGH);
  }
}

void Phase_onGpsAlt() {
  // Skip debounce if max_alt is the disable sentinel.
  if (g_state.max_alt >= MAX_ALT_DEFAULT) return;

  bool over = (g_state.gps_alt > (g_state.max_alt * 1000.0f));
  g_over_threshold[g_over_idx] = over;
  g_over_idx = (g_over_idx + 1) % ABORT_DEBOUNCE_COUNT;
  if (g_over_filled < ABORT_DEBOUNCE_COUNT) g_over_filled++;

  if (g_over_filled < ABORT_DEBOUNCE_COUNT) return;  // window not full yet

  // Only abort when ALL samples in the window are over threshold.
  bool all_over = true;
  for (uint8_t i = 0; i < ABORT_DEBOUNCE_COUNT; ++i) {
    if (!g_over_threshold[i]) { all_over = false; break; }
  }
  if (all_over && !g_state.aborting) {
    Phase_handleAbort(ABORT_GPS_MAX_ALT);
  }
}

void Phase_tick10s() {
  // Roll the pressure-altitude history.
  g_pa[3] = g_pa[2];
  g_pa[2] = g_pa[1];
  g_pa[1] = g_pa[0];
  g_pa[0] = g_state.pressure_alt;

  // VspeedAvg from GPS altitude history (45-second window, 15-second steps).
  if (g_state.gps_alt3 != 0) {
    g_vspeed_avg = ((g_state.gps_alt - g_state.gps_alt1)
                  + (g_state.gps_alt1 - g_state.gps_alt2)
                  + (g_state.gps_alt2 - g_state.gps_alt3)) / 15.0f;
    g_pa_vspeed  = ((g_pa[0] - g_pa[1])
                  + (g_pa[1] - g_pa[2])
                  + (g_pa[2] - g_pa[3])) / 30.0f;
  }
  // GPS stale: fall back to pressure-altitude vspeed.
  if ((millis() - g_state.last_gps_ms) > GPS_STALE_FOR_PA_MS) {
    g_vspeed_avg = ((g_pa[0] - g_pa[1])
                  + (g_pa[1] - g_pa[2])
                  + (g_pa[2] - g_pa[3])) / 30.0f;
  }

  // Initial classification (PHASE_INIT → first real phase).
  if (g_state.phase == PHASE_INIT) {
    if (g_vspeed_avg > -VSPEED_PRELAUNCH_BAND && g_vspeed_avg < VSPEED_PRELAUNCH_BAND) {
      g_state.phase = PHASE_PRELAUNCH;
    } else if (g_vspeed_avg > VSPEED_RISING_THRESH) {
      g_state.phase = PHASE_RISING;
    } else if (g_vspeed_avg < VSPEED_FALLING_THRESH) {
      g_state.phase = PHASE_FALLING;
    }
  }

  // Prelaunch → rising on three consecutive +40 ft GPS samples.
  if (g_state.phase == PHASE_PRELAUNCH && g_state.gps_alt3 != 0) {
    if (((g_state.gps_alt - g_state.gps_alt1) > ALT_RISING_STEP_FT)
     && ((g_state.gps_alt1 - g_state.gps_alt2) > ALT_RISING_STEP_FT)
     && ((g_state.gps_alt2 - g_state.gps_alt3) > ALT_RISING_STEP_FT)) {
      g_state.phase = PHASE_RISING;
      // Note: original used GPS_alt3 (oldest sample) as launch alt.
      // We follow suit.
    }
    // Capture pressure-altitude calibration when truly idle.
    if (g_vspeed_avg > -ALT_CAL_VSPEED_BAND && g_vspeed_avg < ALT_CAL_VSPEED_BAND) {
      g_state.alt_cal = true;
    }
  }

  // Rising → falling on negative vspeed.
  if (g_state.phase == PHASE_RISING && g_vspeed_avg < VSPEED_FALLING_THRESH) {
    g_state.phase = PHASE_FALLING;
  }

  // Falling → landed on near-zero vspeed.
  if (g_state.phase == PHASE_FALLING) {
    if (g_vspeed_avg > -VSPEED_LANDED_BAND && g_vspeed_avg < VSPEED_LANDED_BAND) {
      g_state.phase = PHASE_LANDED;
    }
    // Original code clears the `aborting` flag once we've descended below
    // the max-altitude threshold. Preserve that behavior so the abort
    // logic can rearm if the balloon climbs again.
    if (g_state.gps_alt < g_state.max_alt * 1000.0f) {
      g_state.aborting = false;
    }
  }
}
