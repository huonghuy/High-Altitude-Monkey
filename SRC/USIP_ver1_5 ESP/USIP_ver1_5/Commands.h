// Commands.h
// Console + radio command parser. Handles $ABORT, $MXALT nn, $IDENT, and
// MONKEY nn arriving via either the IDE serial console (Serial) or the
// TinyTrak4 radio (Serial1, dispatched from GPS.cpp).
//
// Wire-protocol invariants:
//   - Numeric ranges: MONKEY nn is 0..84; MXALT nn is 0..99 thousands of ft.
//   - Console form: leading '$' for ABORT/MXALT/IDENT, no '$' for MONKEY.
//   - Radio form:    leading '$' for everything; MONKE truncated for radio
//                    APRS character constraints (legacy).

#ifndef USIP_COMMANDS_H
#define USIP_COMMANDS_H

#include <stdint.h>
#include <stddef.h>

void Commands_pollConsole();

// Called by GPS module when a non-NMEA "$" line arrives via radio.
// `line` points to the '$', `len` is the byte count up to but not
// including any trailing CR/LF.
void Commands_dispatchRadioLine(const char* line, size_t len);

#endif  // USIP_COMMANDS_H
