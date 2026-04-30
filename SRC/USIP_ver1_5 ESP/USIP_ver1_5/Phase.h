// Phase.h
// Flight-phase state machine + abort debounce + cutdown pin handling.
//
// Phase transitions are driven by VspeedAvg, computed from the GPS altitude
// history maintained in g_state.gps_alt{,1,2,3} (a 4-deep ring filled by
// the GPS module on each new fix). When GPS is stale > 21 s, falls back to
// a vertical-speed estimate from the pressure-altitude history pa[4]
// maintained inside this module.
//
// Abort debounce (fix for bug #5 in the original code): rather than the
// old "++/reset" counter that could oscillate forever near max altitude,
// we keep a rolling N-sample window and only fire ABORT when ALL N samples
// are over threshold.

#ifndef USIP_PHASE_H
#define USIP_PHASE_H

#include <stdint.h>

enum AbortReason : uint8_t {
  ABORT_LOCAL_CMD = 0,
  ABORT_RADIO_CMD,
  ABORT_GPS_MAX_ALT
};

// 10-second tick. Recomputes VspeedAvg, runs phase transitions.
void Phase_tick10s();

// Called from the loop on every iteration. Pulses ABORT_pin LOW for
// ENDABORT_PULSE_MS while an abort is active.
void Phase_pinTick();

// Force an abort for the given reason. Drives the cutdown pin.
void Phase_handleAbort(AbortReason reason);

// Called by GPS module on every new altitude fix. Feeds the abort
// debounce ring buffer.
void Phase_onGpsAlt();

#endif  // USIP_PHASE_H
