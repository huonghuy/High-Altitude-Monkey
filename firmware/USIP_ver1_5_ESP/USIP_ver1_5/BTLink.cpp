// BTLink.cpp
#include "BTLink.h"
#include "Config.h"
#include "State.h"
#include "BluetoothSerial.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

namespace {
  BluetoothSerial g_bt;
  uint8_t         g_peer_mac[6] = BT_PEER_MAC_INIT;

  constexpr size_t BT_BUF_LEN = 96;
  char   g_buf[BT_BUF_LEN];
  size_t g_buf_len = 0;
  bool   g_dbg     = true;

  // Reconnect backoff state.
  uint32_t g_last_attempt_ms = 0;
  uint32_t g_backoff_ms      = 5000;     // grows up to 60 s on repeated failure
  constexpr uint32_t BACKOFF_MAX_MS = 60000;

#if STUB_PEER
  // Synthetic reply queue (bench testing without HC-05/Mega).
  struct PendingReply {
    uint32_t fire_at_ms;
    char     payload[40];
  };
  constexpr uint8_t SYNTH_QUEUE_LEN = 4;
  PendingReply g_synth[SYNTH_QUEUE_LEN] = { { 0, "" }, { 0, "" }, { 0, "" }, { 0, "" } };

  // Queue a synth reply. Drops silently if full.
  void synthPush(uint32_t delay_ms, const char* payload) {
    for (uint8_t i = 0; i < SYNTH_QUEUE_LEN; ++i) {
      if (g_synth[i].fire_at_ms == 0) {
        g_synth[i].fire_at_ms = millis() + delay_ms;
        strncpy(g_synth[i].payload, payload, sizeof(g_synth[i].payload) - 1);
        g_synth[i].payload[sizeof(g_synth[i].payload) - 1] = 0;
        return;
      }
    }
  }
#endif

  inline void resetBuf() { g_buf_len = 0; g_buf[0] = 0; }

  // Parse a complete MONKEY reply line. The line begins with "MONKEY".
  void handleReply(const char* line, size_t len) {
    if (len < 9) return;
    // Reply codes are at positions 7-8.
    if (line[7] < '0' || line[7] > '9' || line[8] < '0' || line[8] > '9') return;
    int code = (line[7] - '0') * 10 + (line[8] - '0');

    g_state.last_monkey_reply_ms = millis();

    switch (code) {
      case 10:
      case 11:
        g_state.monkey       = MK_OK;
        g_state.monkey_busy  = false;
        break;
      case 13: {
        g_state.monkey       = MK_OK;
        g_state.monkey_busy  = false;
        // Payload format: "MONKEY 13 vvv,ttt,xxx"
        if (len < 10) break;
        const char* p = line + 9;     // past "MONKEY 13"
        // Skip whitespace.
        while (*p == ' ' && (size_t)(p - line) < len) p++;
        // Three comma-separated ints. Buffer size matches the line buffer
        // so a worst-case payload always fits.
        char tmp[BT_BUF_LEN];
        size_t copy_len = len - (p - line);
        if (copy_len >= sizeof(tmp)) copy_len = sizeof(tmp) - 1;
        memcpy(tmp, p, copy_len);
        tmp[copy_len] = 0;
        char* save = nullptr;
        char* tok  = strtok_r(tmp, ",", &save);
        int   vals[3] = { 0, 0, 0 };
        for (int i = 0; i < 3 && tok; ++i) {
          vals[i] = atoi(tok);
          tok = strtok_r(nullptr, ",", &save);
        }
        g_state.bat_v2  = (float)vals[0] / 10.0f;
        g_state.i_temp2 = (float)vals[1];
        g_state.tx_temp = (float)vals[2];
        if (g_dbg) {
          Serial.print("MONKEY 13 parsed: bat_v2=");
          Serial.print(g_state.bat_v2);
          Serial.print(" i_temp2=");
          Serial.print(g_state.i_temp2);
          Serial.print(" tx_temp=");
          Serial.println(g_state.tx_temp);
        }
        break;
      }
      case 17:
        g_state.monkey      = MK_FAULT;
        g_state.monkey_busy = false;
        Serial1.println("Monkey Processor cannot adjust video.");
        Serial.println("Monkey Processor cannot adjust video.");
        break;
      default:
        if (g_dbg) {
          Serial.print("Unknown MONKEY reply code ");
          Serial.println(code);
        }
        break;
    }
  }

  // Find "MONKEY" in the buffer, dispatch the slice.
  void framedLine() {
    // Strip trailing CR/LF.
    while (g_buf_len > 0 && (g_buf[g_buf_len - 1] == '\n' || g_buf[g_buf_len - 1] == '\r')) {
      g_buf_len--;
    }
    g_buf[g_buf_len] = 0;
    if (g_dbg) {
      Serial.print("===> BT line: ");
      Serial.println(g_buf);
    }
    const char* m = strstr(g_buf, "MONKEY");
    if (m) handleReply(m, g_buf_len - (m - g_buf));
    resetBuf();
  }

  // Append a single character to the buffer; flush on '\n' or overflow.
  void feedChar(char c) {
    if (g_buf_len + 1 >= BT_BUF_LEN) {
      resetBuf();
      return;
    }
    g_buf[g_buf_len++] = c;
    g_buf[g_buf_len]   = 0;
    if (c == '\n') framedLine();
  }
}

void BT_init() {
#if STUB_PEER
  Serial.println("BT: STUB_PEER mode — no real Bluetooth.");
  g_state.bt = BT_CONNECTED;
  return;
#else
  Serial.println("BT: starting in master mode (RFCOMM/SPP, classic).");
  g_bt.begin(BT_MASTER_NAME, true);
  g_bt.setPin(BT_PIN_CSTR, strlen(BT_PIN_CSTR));

  // Initial connect attempts. Each call blocks 2-10 s — feed the watchdog
  // around it (stubbed for now; wire up esp_task_wdt_reset() if WDT is on).
  for (uint8_t attempt = 1; attempt <= BT_CONNECT_ATTEMPTS; ++attempt) {
    Serial.print("BT: attempt ");
    Serial.print(attempt);
    Serial.println(" connecting to HC-05...");
    g_state.bt = BT_CONNECTING;
    if (g_bt.connect(g_peer_mac)) {
      Serial.println("BT: connected.");
      g_state.bt = BT_CONNECTED;
      g_backoff_ms = 5000;
      return;
    }
    Serial.println("BT: connect failed, retrying...");
    delay(BT_CONNECT_RETRY_DELAY_MS);
  }
  Serial.println("BT: initial connect failed; will retry from BT_reconnectTick().");
  g_state.bt = BT_DISCONNECTED;
  g_last_attempt_ms = millis();
#endif
}

void BT_pollRx() {
#if STUB_PEER
  // Drain the synth queue: any entries whose fire_at_ms has elapsed get
  // pushed into the framer just like real bytes.
  uint32_t now = millis();
  for (uint8_t i = 0; i < SYNTH_QUEUE_LEN; ++i) {
    if (g_synth[i].fire_at_ms != 0 && (int32_t)(now - g_synth[i].fire_at_ms) >= 0) {
      const char* p = g_synth[i].payload;
      while (*p) feedChar(*p++);
      feedChar('\n');
      g_synth[i].fire_at_ms = 0;
    }
  }
#else
  while (g_bt.available()) feedChar((char)g_bt.read());
#endif
}

void BT_reconnectTick() {
#if STUB_PEER
  return;
#else
  if (g_state.bt == BT_CONNECTED) {
    if (!g_bt.connected()) {
      Serial.println("BT: link lost.");
      g_state.bt = BT_DISCONNECTED;
      g_last_attempt_ms = millis();
    }
    return;
  }
  if (g_state.bt == BT_CONNECTING) return;     // re-entry guard
  if ((uint32_t)(millis() - g_last_attempt_ms) < g_backoff_ms) return;

  Serial.print("BT: reconnect attempt (backoff ");
  Serial.print(g_backoff_ms);
  Serial.println(" ms)...");
  g_state.bt = BT_CONNECTING;
  g_last_attempt_ms = millis();
  if (g_bt.connect(g_peer_mac)) {
    Serial.println("BT: reconnected.");
    g_state.bt   = BT_CONNECTED;
    g_backoff_ms = 5000;
  } else {
    Serial.println("BT: reconnect failed.");
    g_state.bt   = BT_DISCONNECTED;
    g_backoff_ms = (g_backoff_ms * 2 < BACKOFF_MAX_MS) ? g_backoff_ms * 2 : BACKOFF_MAX_MS;
  }
#endif
}

bool BT_sendMonkey(uint8_t cmd) {
  if (cmd > 84) {
    // Out of valid range. Original code silently rejects, we do too.
    if (g_dbg) {
      Serial.print("BT_sendMonkey: out-of-range cmd ");
      Serial.println(cmd);
    }
    return false;
  }
  char line[16];
  snprintf(line, sizeof(line), "MONKEY %02u\n", (unsigned)cmd);
  if (g_dbg) {
    Serial.print("BT TX: ");
    Serial.print(line);
  }

#if STUB_PEER
  g_state.monkey      = MK_BUSY;
  g_state.monkey_busy = true;
  // Synth replies that mirror the real airborne behavior.
  if      (cmd == 12) synthPush(200,  "MONKEY 13 73,21,28");
  else if (cmd == 9)  synthPush(200,  "MONKEY 11");
  else if (cmd == 99) synthPush(500,  "MONKEY 17");          // never reachable in normal range
  else                synthPush(1500, "MONKEY 11");          // sim servo motion time
  return true;
#else
  if (g_state.bt != BT_CONNECTED) {
    if (g_dbg) Serial.println("BT_sendMonkey: not connected, dropping.");
    return false;
  }
  g_bt.print(line);
  g_state.monkey      = MK_BUSY;
  g_state.monkey_busy = true;
  return true;
#endif
}

void BT_pollMonkeyOk() {
  if (g_state.monkey == MK_OK && !g_state.monkey_busy) {
    BT_sendMonkey(12);
  }
}

void BT_pollMonkeyDown() {
  if (g_state.monkey != MK_OK) {
    BT_sendMonkey(9);
  }
  // Demote to MK_UNKNOWN if last reply has aged out.
  if (g_state.monkey == MK_OK
      && (millis() - g_state.last_monkey_reply_ms) > MONKEY_REPLY_TIMEOUT_MS) {
    g_state.monkey = MK_UNKNOWN;
  }
}
