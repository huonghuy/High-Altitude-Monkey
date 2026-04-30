// State.h
// Cross-module mission state. Module-private state (rx buffers, parser
// scratch, BME instance, etc.) lives inside each module's .cpp.
//
// Replaces the ~13 boolean flags that lived at file scope in the original
// USIP_ver1_5.ino. Each former bool maps to one of the enums below or to
// a bool field on State (only for genuinely 2-valued things).

#ifndef USIP_STATE_H
#define USIP_STATE_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------

// FlightPhase is the natural phase machine. It is INDEPENDENT of the
// abort flag — aborting is signalled by State.aborting (a separate bool)
// while the phase continues evolving naturally underneath. This mirrors
// the original code, where `aborting` was a separate flag from
// prelaunch/rising/falling/landed.
enum FlightPhase : uint8_t {
  PHASE_INIT = 0,    // BME/BT not yet up; phase classifier hasn't run
  PHASE_PRELAUNCH,
  PHASE_RISING,
  PHASE_FALLING,
  PHASE_LANDED
};
const char* phaseName(FlightPhase p);

enum TimeSyncState : uint8_t {
  TS_NEVER = 0,    // no GPS ever seen
  TS_LIVE          // GPS time has been observed at least once
};

enum BTState : uint8_t {
  BT_DISCONNECTED = 0,
  BT_CONNECTING,    // SerialBT.connect() in progress (blocks 2-10 s)
  BT_CONNECTED
};

enum MonkeyState : uint8_t {
  MK_UNKNOWN = 0,   // no reply ever / replies stale
  MK_BUSY,          // command sent, awaiting reply
  MK_OK,            // last reply MONKEY 10/11/13 within timeout
  MK_FAULT          // last reply MONKEY 17 (cannot adjust video)
};

enum GpsValidity : uint8_t {
  GPS_INVALID = 0,
  GPS_VALID
};

// -----------------------------------------------------------------------------
// State struct
// -----------------------------------------------------------------------------

struct State {
  // Flight machine
  FlightPhase  phase;
  bool         alt_cal;          // pressure-altitude calibration captured
  bool         comms_ok;         // currently always true; reserved for future
  bool         bme_ok;           // BME280 init succeeded

  // Time sync + mission clock (filled by Time module)
  TimeSyncState time_sync;
  uint32_t      time_sync_at_ms;
  int  gps_hr, gps_min, gps_sec; // from GPGGA
  int  mis_hr, mis_min, mis_sec; // mission elapsed; never wraps hour
  uint32_t next_sec_ms;          // millis() at which to advance the second
  uint32_t last_gps_ms;          // millis() of last good GPS fix

  // GPS fix
  GpsValidity gps;
  float gps_alt;                 // current altitude (feet)
  float gps_alt1, gps_alt2, gps_alt3;   // history at t-15, t-30, t-45 s

  // BT / monkey link
  BTState     bt;
  MonkeyState monkey;
  uint32_t    last_monkey_reply_ms;
  bool        monkey_busy;

  // Sensor cache (filled by Sensors module)
  float ps_temp;        // BME280 temperature (deg C)
  float ps_pressure;    // BME280 pressure (Pa)
  float pressure_alt;   // BME280-derived altitude (feet)
  float i_temp1;        // inside temp from analog ADC (FLIGHT) or default (GROUND)
  float i_temp2;        // inside temp reported by airborne in MONKEY 13
  float o_temp;         // outside temp from analog ADC (FLIGHT) or default (GROUND)
  float tx_temp;        // transmitter temp reported by airborne in MONKEY 13
  float bat_v1;         // ground battery (FLIGHT analog or GROUND default)
  float bat_v2;         // airborne battery from MONKEY 13

  // Abort window. `aborting` is the abstract state (we are in an
  // emergency-cutdown attempt) and is independent of the natural phase.
  // `abort_pin_pulsing` is the hardware-level fact that the cutdown pin
  // is currently asserted LOW. Pulse ends after ENDABORT_PULSE_MS but
  // `aborting` stays true until the phase machine clears it (when
  // descending below max_alt).
  float    max_alt;             // thousands of feet; MAX_ALT_DEFAULT disables
  uint32_t end_abort_ms;        // millis() at which to release the cutdown pin
  bool     aborting;            // emergency cutdown in progress
  bool     abort_pin_pulsing;   // cutdown pin currently LOW
};

// Single shared instance. Defined in State.cpp.
extern State g_state;

// Reset to safe defaults. Call once from setup().
void State_init();

#endif  // USIP_STATE_H
