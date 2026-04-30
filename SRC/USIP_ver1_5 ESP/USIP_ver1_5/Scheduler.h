// Scheduler.h
// Tiny cooperative tick scheduler. Replaces the scattered
// `if (millis() >= looptimeN + interval) { ... }` blocks in the original
// USIP_ver1_5.ino main loop.
//
// Usage:
//   void tick1Hz()  { ... }
//   void tick60s()  { ... }
//   setup()  { Scheduler_register(1000, tick1Hz);
//              Scheduler_register(60000, tick60s); ... }
//   loop()   { Scheduler_tick(); ... }
//
// Cadence is "no faster than interval_ms"; uses subtraction-based timing
// so it survives the millis() rollover at ~49 days.
//
// The 100 ms `time_update(0)` and the mission-clock-driven APRS bursts
// stay outside the scheduler — they have irregular cadences that don't
// fit a fixed-interval model.

#ifndef USIP_SCHEDULER_H
#define USIP_SCHEDULER_H

#include <stdint.h>

typedef void (*TickFn)(void);

// Returns true on success, false if the task table is full.
bool Scheduler_register(uint32_t interval_ms, TickFn fn);

// Call once per loop() iteration. Runs every task whose interval has elapsed.
void Scheduler_tick();

#endif  // USIP_SCHEDULER_H
