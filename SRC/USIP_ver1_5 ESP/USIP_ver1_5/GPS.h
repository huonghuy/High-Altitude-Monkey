// GPS.h
// NMEA framer on Serial1 RX (the TinyTrak4 radio's data channel).
// Parses $GPGGA (time, position, altitude, fix quality) and $GPRMC (date),
// validates the XOR checksum with explicit bounds checks, and dispatches
// non-NMEA radio commands ($ABORT, $MXALT, $IDENT, $MONKE) to Commands.h.
//
// `GPS_pollRx()` must be called from loop() every iteration (the
// `serialEvent1` auto-dispatch is unreliable on ESP-32; same fix as
// pollSerialBT in Phase 1).

#ifndef USIP_GPS_H
#define USIP_GPS_H

#include <stdint.h>

void GPS_pollRx();

#endif  // USIP_GPS_H
