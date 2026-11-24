// Time.h
// Mission clock + GPS time alignment.
//
// `Time_tick()` is called from loop() every iteration. When the mission
// second elapses, it advances the clock and fires any APRS bursts that
// should land on the new (min,sec) pair.
//
// `Time_onGpsFix()` is called by GPS module when a valid GPGGA fix arrives,
// so the mission clock can re-synchronize to GPS seconds.

#ifndef USIP_TIME_H
#define USIP_TIME_H

#include <stdint.h>

// One-time init. Sets next_sec_ms = millis() + 1000.
void Time_init();

// Call once per loop() iteration. Advances mission clock when next_sec_ms
// elapses. Cheap when nothing to do.
void Time_tick();

// Called by GPS module when a valid fix arrives. Aligns mission clock to
// GPS seconds. `at_ms` is the millis() reading at receipt of the fix.
void Time_onGpsFix(int gps_hr, int gps_min, int gps_sec, uint32_t at_ms);

#endif  // USIP_TIME_H
