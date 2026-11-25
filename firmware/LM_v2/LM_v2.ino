// High-Altitude-Monkey / LM V2 - bench bring-up firmware, NOT flight release.
// Arduino-ESP32 target: ESP32 Dev Module (original ESP32, not S3/C3).
// Mission Control link: 19200 8N1, using external MAX3232E; Bluetooth Classic SPP.
// Transparent telemetry bridge: preserves bytes, including CR/LF and ANSI escapes.
// USB is diagnostic receive-only by design; no implicit injection of USB commands.
// Existing legacy telemetry parser is supplied separately as a migration reference.
#include <Arduino.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include "Config.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED) || !defined(CONFIG_BT_SPP_ENABLED)
#error "Select an original ESP32 with Bluetooth Classic SPP enabled."
#endif

BluetoothSerial terminal;
HardwareSerial mission(2);

namespace {
uint32_t lastTraffic = 0;
uint32_t usbDropped = 0;
uint32_t btDropped = 0;
uint32_t mcRxBytes = 0;
uint32_t mcTxBytes = 0;
uint8_t data[64];

void recordActivity() {
  lastTraffic = millis();
  digitalWrite(lm::activityLed, HIGH);
}

void pumpMission() {
  size_t count = 0;
  while (mission.available() && count < sizeof(data)) {
    int b = mission.read();
    if (b < 0) break;
    data[count++] = static_cast<uint8_t>(b);
  }
  if (!count) return;
  mcRxBytes += count;
  recordActivity();
  // Do not block the mission UART waiting for the USB serial console.
  size_t n = static_cast<size_t>(Serial.availableForWrite());
  if (n > count) n = count;
  if (n) n = Serial.write(data, n);
  usbDropped += count - n;
  if (terminal.hasClient()) {
    // BluetoothSerial::write may block on a congested link; bench-test this
    // before enabling the bridge in a time-critical mission environment.
    size_t sent = terminal.write(data, count);
    btDropped += count - sent;
  }
}

void pumpBluetooth() {
  if (!terminal.hasClient()) return;
  // Forward bytes as received; the ground-control firmware handles framing.
  // Only Bluetooth is a command source in this bench firmware.
  int freeBytes = mission.availableForWrite();
  for (size_t i = 0; i < sizeof(data) && freeBytes > 0 && terminal.available(); ++i) {
    int b = terminal.read();
    if (b < 0) break;
    if (mission.write(static_cast<uint8_t>(b)) == 1) ++mcTxBytes;
    --freeBytes;
    recordActivity();
  }
}
}

void setup() {
  pinMode(lm::statusLed, OUTPUT);
  pinMode(lm::activityLed, OUTPUT);
  digitalWrite(lm::statusLed, LOW);
  digitalWrite(lm::activityLed, LOW);
  Serial.begin(lm::usbBaud);
  WiFi.mode(WIFI_OFF);
  mission.setRxBufferSize(2048);
  mission.begin(lm::missionBaud, SERIAL_8N1, lm::missionRx, lm::missionTx);
  // No fixed PIN or promise of authenticated control: use a controlled bench.
  // Production command authorization/pairing policy must be defined separately.
  if (!terminal.begin(lm::bluetoothName)) {
    Serial.println("LM V2: Bluetooth SPP initialization failed");
  }
  Serial.println("LM V2 bench bridge: 19200 8N1; Bluetooth commands; USB monitor only");
}

void loop() {
  digitalWrite(lm::statusLed, terminal.hasClient() ? HIGH : LOW);
  pumpMission();
  pumpBluetooth();
  if (millis() - lastTraffic > 80) digitalWrite(lm::activityLed, LOW);
  // Ignore USB input; this avoids an unintended second command source.
  for (size_t i = 0; i < sizeof(data) && Serial.available(); ++i) Serial.read();
  delay(1);
}
