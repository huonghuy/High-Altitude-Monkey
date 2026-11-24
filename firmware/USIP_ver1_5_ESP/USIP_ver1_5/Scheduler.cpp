// Scheduler.cpp
#include "Scheduler.h"
#include <Arduino.h>

namespace {
  constexpr uint8_t MAX_TASKS = 12;

  struct Task {
    uint32_t interval_ms;
    uint32_t last_run_ms;
    TickFn   fn;
  };

  Task    g_tasks[MAX_TASKS];
  uint8_t g_task_count = 0;
}

bool Scheduler_register(uint32_t interval_ms, TickFn fn) {
  if (g_task_count >= MAX_TASKS || fn == nullptr) return false;
  g_tasks[g_task_count].interval_ms = interval_ms;
  g_tasks[g_task_count].last_run_ms = millis();
  g_tasks[g_task_count].fn          = fn;
  g_task_count++;
  return true;
}

void Scheduler_tick() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < g_task_count; ++i) {
    Task& t = g_tasks[i];
    if ((uint32_t)(now - t.last_run_ms) >= t.interval_ms) {
      t.last_run_ms = now;
      t.fn();
    }
  }
}
