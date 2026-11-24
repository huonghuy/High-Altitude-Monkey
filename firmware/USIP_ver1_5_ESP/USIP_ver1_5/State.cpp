// State.cpp
#include "State.h"
#include "Config.h"

State g_state;

void State_init() {
  g_state.phase             = PHASE_INIT;
  g_state.alt_cal           = false;
  g_state.comms_ok          = true;
  g_state.bme_ok            = false;

  g_state.time_sync         = TS_NEVER;
  g_state.time_sync_at_ms   = 0;
  g_state.gps_hr            = 0;
  g_state.gps_min           = 0;
  g_state.gps_sec           = 0;
  g_state.mis_hr            = 0;
  g_state.mis_min           = 0;
  g_state.mis_sec           = 0;
  g_state.next_sec_ms       = 1000;
  g_state.last_gps_ms       = 0;

  g_state.gps               = GPS_INVALID;
  g_state.gps_alt           = 0.0f;
  g_state.gps_alt1          = 0.0f;
  g_state.gps_alt2          = 0.0f;
  g_state.gps_alt3          = 0.0f;

  g_state.bt                = BT_DISCONNECTED;
  g_state.monkey            = MK_UNKNOWN;
  g_state.last_monkey_reply_ms = 0;
  g_state.monkey_busy       = false;

  // Sensor defaults — used in GROUND mode and as initial values until
  // FLIGHT-mode sensors come online.
  g_state.ps_temp           = 18.0f;
  g_state.ps_pressure       = 101325.0f;
  g_state.pressure_alt      = 0.0f;
  g_state.i_temp1           = 13.0f;
  g_state.i_temp2           = 13.0f;
  g_state.o_temp            = -44.9f;
  g_state.tx_temp           = 65.4f;
  g_state.bat_v1            = 7.3f;
  g_state.bat_v2            = 14.5f;

  g_state.max_alt           = MAX_ALT_DEFAULT;
  g_state.end_abort_ms      = 0;
  g_state.aborting          = false;
  g_state.abort_pin_pulsing = false;
}

const char* phaseName(FlightPhase p) {
  switch (p) {
    case PHASE_INIT:      return "INIT";
    case PHASE_PRELAUNCH: return "PRELAUNCH";
    case PHASE_RISING:    return "RISING";
    case PHASE_FALLING:   return "FALLING";
    case PHASE_LANDED:    return "LANDED";
  }
  return "?";
}
