# LM_v2 firmware

Byte-for-byte bridge between Mission Control's Telemetry Processor port (MC `Serial2`, RS-232 at 19200 8N1) and Bluetooth Classic SPP. USB is a read-only mirror for debugging at 115200 baud. Board: `hardware/kicad/LM_v2`.

## Build

Arduino IDE, or the CLI bundled with it:

```
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app firmware/LM_v2
```

- **Board:** "ESP32 Dev Module", the original ESP32. ESP32-S3 and C3 have no Bluetooth Classic and cannot talk to an HC-05.
- **Partition scheme:** "Huge APP (3MB No OTA/1MB SPIFFS)". The Bluetooth Classic stack makes the sketch 1.6 MB, which overflows the default 1.3 MB app partition. The ESP32-WROOM-32E-N4 has 4 MB of flash.
- **Core:** arduino-esp32 **3.x**. Built clean on 3.3.12 (2026-11-25): 1.6 MB flash (50%), 62 KB RAM (18%). `BluetoothSerial` is deprecated and will not be in core 4.0 by default, so stay on 3.x until that's dealt with.
- **Upload:** USB-C through the on-board CH340C. The auto-reset circuit puts the ESP32 into download mode by itself; if it doesn't, hold BOOT and tap RESET.

## Pins (match the board)

| Signal | ESP32 GPIO | Module pin |
|---|---|---|
| Mission Control RX (from MAX3232) | IO16 | 27 |
| Mission Control TX (to MAX3232) | IO17 | 28 |
| Status LED (D1): Bluetooth client connected | IO25 | 10 |
| Activity LED (D2): traffic | IO26 | 11 |

Bluetooth name: `HAM-LM-V2`. Wi-Fi is off.

## Before using it on a mission

- **Pairing direction.** This sketch is the Bluetooth *slave*: it waits for a client. The comms payload's HC-05 must therefore be set as master and bound to this ESP32's Bluetooth address, which you can do with the HC-05 Programmer. The other option is to change the sketch to connect out to the HC-05 as master.
- **No telemetry display yet.** The old Telemetry Processor parsed telemetry and drew the terminal screen; this sketch only passes bytes through. The original code is in `reference/Telemetry_Processor.ino.txt` for porting.
- **Not yet tested on hardware.** Test on the bench with a loopback or a simulated Mission Control first. Bluetooth commands are not authenticated, and Bluetooth writes can block when the link is congested.
