// Telemetry.h
// 60 s APRS telemetry packet to Serial1 (radio) and a mirror on Serial.
// 20-min PARM/UNIT/EQU/BIT metadata bursts driven by the mission clock.
// IDENT and MXALT replies (operator-driven status reports).
//
// The telemetry packet shape is FIXED — the upstream APRS decoder depends
// on a specific field count and encoding. Both FLIGHT and GROUND mode
// emit identical-shape packets; GROUND just substitutes sentinel values
// where the FLIGHT-only analog sensors would supply data.

#ifndef USIP_TELEMETRY_H
#define USIP_TELEMETRY_H

#include <stdint.h>

void Telemetry_tick60s();

// Burst-flag setters. Called by Time module when the mission clock
// crosses the canonical (min, sec) burst windows. The actual sends happen
// from Telemetry_drainBursts(), called every loop iteration to flush any
// pending bursts onto Serial1.
void Telemetry_requestParm();
void Telemetry_requestUnit();
void Telemetry_requestEqu();
void Telemetry_requestBit();
void Telemetry_drainBursts();

// Operator-command responses.
void Telemetry_processIdent();
void Telemetry_processMxAlt();

#endif  // USIP_TELEMETRY_H
