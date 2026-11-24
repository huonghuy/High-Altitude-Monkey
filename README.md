# High-Altitude-Monkey (HAM)

HAM is a robotic monkey that flies on a high-altitude balloon. It moves, talks and
streams live video during flight, and is used for K-12 "near space" outreach on the
University of Bridgeport STEM bus. The project started in 2018 under a NASA USIP grant.
The 2024–2025 rebuild (HARM) adds new electronics, firmware and a controllable vent.
See the [ASEE 2025 paper](docs/papers/ASEE-2025/) for background.

## System overview

```
 Command Center ──(MC board)── Mega + HC-05 ))) Bluetooth ((( HC-05 + Mega ── ham radio ─┐
   (ground)                                                    (comms payload,          │
                                                                balloon)                │
                                                                                        │
                                                     ham-radio packets = commands       │
                                                                                        ▼
                                   HAM payload box ── Main board (Mega 2560 Pro)
                                     ├─ PCA9685 → servos (monkey movement)
                                     ├─ HC-05 ← commands from the comms payload
                                     ├─ 12 V switch → VideoLynx VM-70X video TX (on/off)
                                     └─ VL board on the VM-70X (RF power, camera, fan)
```

| Board | What it does | KiCad project |
|-------|--------------|---------------|
| **Main** | Primary board in the monkey box. Drives the servos, receives commands over Bluetooth from the comms payload, switches and controls the VideoLynx. | `hardware/kicad/Main` |
| **LM** (Land Module) | Used on the ground when HAM isn't flying. Connects the payload box to the command center over Bluetooth. | `hardware/kicad/LM` |
| **MC** (Mission Control) | Replacement for the old mission-control board in the command center. | `hardware/kicad/MC` |
| **VL** (VideoLynx) | Small board on top of the VM-70X video transmitter. Wired to Main with jumpers. | `hardware/kicad/VL` |
| **HC-05 Programmer** | Utility board for configuring the HC-05 Bluetooth modules. | `hardware/kicad/HC-05_Programmer` |

The comms payload (ham radio + Mega + HC-05) has no custom PCB.

**No HAM PCB has worked yet.** Read [`hardware/KNOWN_ISSUES.md`](hardware/KNOWN_ISSUES.md)
before ordering boards.

## Repository layout

```
hardware/
  kicad/            KiCad 10 projects (current source of truth for PCBs)
  easyeda-archive/  Original EasyEDA Standard files, all versions (read-only history)
  gerbers/          Fab outputs from past orders
  breadboard/       Early breadboard layout
  datasheets/       Part datasheets and module pinouts
  bom/              Parts list
  KNOWN_ISSUES.md   Open hardware bugs
firmware/
  Monkey_v2/            Flight firmware for the monkey (Main board, Mega 2560)
  Ground_Control/       Ground-station control sketch
  Telemetry_Processor/  Ground-side telemetry/APRS processing
  USIP_ver1_5_ESP/      ESP32 ground-station firmware (current)
  archive/              Older firmware versions (2019 USIP code, Monkey v1, USIP 1.1–1.4)
mechanical/         Body/arm STLs, MC panel (SolidWorks, 3MF, DXF)
docs/
  papers/ASEE-2025/   LaTeX source and PDF of the ASEE paper
  presentations/      Posters and slide decks (BERC 2024, NASA Minnesota, ASEE)
  flight-data/        Data from the 2025-06-05 flight
  images/             Photos, renders, diagrams
  usip-notes/         2019 USIP build notes and FAA documents
```

## Getting started

**Hardware:** Install [KiCad 10](https://www.kicad.org/) and open the `.kicad_pro` in
`hardware/kicad/<board>/`.

**Firmware (Mega boards):** Arduino IDE 2.x. Install the libraries listed in
`firmware/Monkey_v2/Libraries.txt` (Adafruit PWM Servo Driver, Adafruit BusIO), plus the
CH340 driver if you're using the Mega 2560 Pro module.

**Firmware (ESP32 ground station):** Arduino IDE with the ESP32 core. Needs Adafruit
BME280 and Adafruit Unified Sensor. See the notes at the top of
`firmware/USIP_ver1_5_ESP/USIP_ver1_5/Config.h`.

## Large files

Slide decks (`*.pptx`) are stored with [Git LFS](https://git-lfs.com). Run
`git lfs install` before cloning, or you'll get small pointer files instead of the decks.

## License

See [LICENSE](LICENSE).
