// Commands.cpp
#include "Commands.h"
#include "Config.h"
#include "State.h"
#include "BTLink.h"
#include "Phase.h"
#include "Telemetry.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

namespace {
  // Console (IDE serial) line buffer. The original code reserved 15 bytes;
  // 16 with NUL terminator gives the same capacity.
  constexpr size_t CONSOLE_BUF_LEN = 16;
  char    g_console[CONSOLE_BUF_LEN];
  size_t  g_console_len = 0;
  bool    g_dbg         = true;

  inline void resetConsole() { g_console_len = 0; g_console[0] = 0; }

  // Read 2 ASCII digits at offset and return value, or -1 if not digits.
  int readTwoDigits(const char* s, size_t off, size_t len) {
    if (off + 1 >= len) return -1;
    if (!isdigit((unsigned char)s[off]))     return -1;
    if (!isdigit((unsigned char)s[off + 1])) return -1;
    return (s[off] - '0') * 10 + (s[off + 1] - '0');
  }

  // Process a console-form line (no leading '$' for MONKEY, '$' for others).
  // The string is NUL-terminated and `len` excludes any trailing CR/LF.
  void processConsole(const char* s, size_t len) {
    if (len < 6 || len >= 12) {
      Serial.print(s);
      Serial.print(": Unknown cmd bad size  ?? (");
      Serial.print((unsigned)len);
      Serial.println(")");
      return;
    }

    // Match the 6-char prefix in upper-case (preserve original behavior:
    // accepted both "$abort" and "$ABORT").
    char head[7];
    for (size_t i = 0; i < 6 && i < len; ++i) head[i] = (char)toupper((unsigned char)s[i]);
    head[6] = 0;

    if (strcmp(head, "$ABORT") == 0) {
      Phase_handleAbort(ABORT_LOCAL_CMD);
      return;
    }
    if (strcmp(head, "$MXALT") == 0) {
      // "$MXALT NN" — read 2 digits at offset 7.
      int v = readTwoDigits(s, 7, len);
      if (v >= 0) {
        g_state.max_alt = (float)v;
        Telemetry_processMxAlt();
      } else {
        Serial.println("$MXALT: bad number");
      }
      return;
    }
    if (strcmp(head, "$IDENT") == 0) {
      Telemetry_processIdent();
      return;
    }
    if (strcmp(head, "MONKEY") == 0) {
      // "MONKEY NN" — read 2 digits at offset 7.
      int v = readTwoDigits(s, 7, len);
      if (v < 0 || v > 84) {
        Serial.print("Bad monkey_cmd: ");
        Serial.println(s);
        return;
      }
      if (g_dbg) {
        Serial.print("Console MONKEY ");
        Serial.println(v);
      }
      BT_sendMonkey((uint8_t)v);
      return;
    }
    Serial.print(head);
    Serial.println(": Unknown cmd ok size ??");
  }
}

void Commands_pollConsole() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (g_console_len + 1 >= CONSOLE_BUF_LEN) {
      // Overflow.
      resetConsole();
      continue;
    }
    g_console[g_console_len++] = c;
    g_console[g_console_len]   = 0;
    if (c == '\n') {
      // Strip trailing CR/LF.
      size_t plen = g_console_len;
      while (plen > 0 && (g_console[plen - 1] == '\n' || g_console[plen - 1] == '\r')) plen--;
      g_console[plen] = 0;
      processConsole(g_console, plen);
      resetConsole();
    }
  }
}

void Commands_dispatchRadioLine(const char* line, size_t len) {
  // Radio-form lines start with '$' and use the body after the '$'.
  // The original code matched the first 5 chars ("ABORT", "MXALT", "IDENT",
  // "MONKE") because the APRS source truncates one character. We match
  // the same way for backward compat.
  if (len < 6 || line[0] != '$') {
    Serial1.println("USIP: Not Valid Command");
    return;
  }
  const char* body = line + 1;
  size_t body_len  = len - 1;

  if (body_len >= 5 && memcmp(body, "ABORT", 5) == 0) {
    Phase_handleAbort(ABORT_RADIO_CMD);
    return;
  }
  if (body_len >= 5 && memcmp(body, "MXALT", 5) == 0) {
    // Body indices 5='space', 6,7=digits.
    int v = readTwoDigits(body, 6, body_len);
    if (v >= 0) {
      g_state.max_alt = (float)v;
      Telemetry_processMxAlt();
    }
    return;
  }
  if (body_len >= 5 && memcmp(body, "IDENT", 5) == 0) {
    if (g_dbg) Serial.println("GOT ident COMMAND VIA RADIO");
    Telemetry_processIdent();
    return;
  }
  if (body_len >= 5 && memcmp(body, "MONKE", 5) == 0) {
    if (g_dbg) Serial.println("GOT MONKEY COMMAND VIA RADIO");
    int v = readTwoDigits(body, 6, body_len);
    if (v >= 0 && v < 85) {
      BT_sendMonkey((uint8_t)v);
    }
    return;
  }
  Serial1.println("USIP: Not Valid Command");
}
