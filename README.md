# Zigbee Touch Keypad

A battery-powered, wall-mountable touch keypad built around the **ESP32-C6** and an **MPR121** capacitive touch controller. Enter a numeric code on the 12-key pad and the device reports it over **Zigbee** to your smart home — ideal for door codes, alarm panels, or any automation that needs a physical PIN input.

This repository ships a complete **Zigbee** example firmware. The same ESP32-C6 hardware is equally suited for **Matter** and **Thread** — swap the radio stack and adapt the application layer while keeping the touch input and power-management logic.

## Gallery

### Installed

<p align="center">
  <img src="touch_matrix_real.jpg" alt="Keypad mounted on a wall next to a door" width="480">
</p>

<p align="center">
  <img src="touch_matrix%20v17_1a.jpg" alt="Keypad on a concrete wall beside a black door" width="480">
  <img src="touch_matrix%20v17_1b.jpg" alt="Rendered view of the keypad on a concrete wall" width="480">
</p>

### Exploded view & internals

<p align="center">
  <img src="touch_matrix%20v19.jpg" alt="Exploded view: PCB back, internals with battery and ESP32-C6, and keypad overlay" width="720">
</p>

<p align="center">
  <img src="touch_matrix%20v19a.jpg" alt="Exploded assembly on a wooden surface" width="480">
  <img src="touch_matrix%20v19b.jpg" alt="Components laid out next to a banana for scale" width="480">
</p>

## Features

- **12 capacitive touch keys** — numbers 0–9, clear, and enter
- **Zigbee end device** — joins a Zigbee 3.0 network and reports passcodes as analog input clusters
- **Matter & Thread ready** — ESP32-C6 supports all three protocols; this repo demonstrates Zigbee
- **Battery powered** — 3.7 V / 1300 mAh LiPo with voltage and state-of-charge reporting over Zigbee
- **~6 months on battery** — aggressive deep-sleep between touches keeps average draw low enough for roughly half a year of typical use
- **Low power** — enters deep sleep after 10 s of inactivity; wakes on touch via MPR121 interrupt
- **Status LED** — the onboard LED shines through the translucent case to show when the device wakes up and when a button is pressed
- **Audible feedback** — short beep on each key press
- **Factory reset** — hold the BOOT button for 3 s to reset Zigbee pairing
- **Production-ready PCB** — Gerber files included; upload directly to manufacturers such as [JLCPCB](https://jlcpcb.com)

## Hardware

| Component | Description |
|-----------|-------------|
| MCU | [ESP32-C6 DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/index.html) |
| Touch sensor | [Adafruit MPR121](https://www.adafruit.com/product/1982) (12-channel capacitive touch, I²C) |
| Power | 3.7 V LiPo (1300 mAh), USB-C charging |
| Enclosure | Custom PCB backplane + 3D-printed case (`case_mid_final.stl`) |

### Pin mapping

| Signal | GPIO | Notes |
|--------|------|-------|
| I²C SDA | 18 | MPR121 data |
| I²C SCL | 4 | MPR121 clock |
| IRQ | 6 | MPR121 interrupt (deep-sleep wake) |
| Battery ADC | 2 | Voltage divider input |
| Buzzer | 15 | Key-press feedback |
| BOOT button | 9 | Factory reset (hold 3 s) |

The touch matrix PCB exposes a 5-pin header (**GND, INT, SCL, SDA, 3.3 V**) for connecting the MPR121 breakout.

### Key layout

The 12 MPR121 channels map to the front-panel keys:

| Channel | Key |
|---------|-----|
| 0–8 | 1 – 9 |
| 9 | 0 |
| 10 | Clear |
| 11 | Enter |

Up to **8 digits** can be entered before the buffer is full.

### PCB manufacturing

Gerber files for the touch-matrix PCB are bundled in `touch_matrix_Y24.zip` (latest revision) and `touch_matrix_Y23.zip`. Extract the archive and upload the Gerbers to any PCB fab — they are ready to order as-is from [JLCPCB](https://jlcpcb.com) and similar services without further conversion.

## Wireless integration

### Zigbee (included example)

The device registers as a Zigbee **end device** with manufacturer **DrDoms** and model **KeyPad2**. Two endpoints are exposed:

| Endpoint | Cluster | Description |
|----------|---------|-------------|
| 1 | Analog Input + Battery | Passcode entry and battery status |
| 2 | Analog Input | Battery state of charge (%) |

**Passcode flow**

1. User enters digits and presses **Enter**.
2. The entered value is reported on endpoint 1 as an analog input (`Passcode`).
3. Battery voltage and percentage are reported at the same time.
4. After 2 s the passcode is reset to **0** (clears the reported value for the next entry).

Pair the device with any Zigbee 3.0 coordinator (ZHA, Zigbee2MQTT, etc.) and bind to the analog input clusters.

### Matter & Thread

The ESP32-C6 radio supports **802.15.4**, making it a natural platform for **Thread** and **Matter** end devices as well. The touch handling, battery monitoring, and deep-sleep logic in `main.cpp` are protocol-agnostic — only the reporting layer needs to change. Espressif's [ESP-Matter](https://github.com/espressif/esp-matter) SDK provides a starting point for a Matter port.

## Building & flashing

This project uses [PlatformIO](https://platformio.org/).

### Prerequisites

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) or the PlatformIO IDE extension
- USB connection to the ESP32-C6 DevKit

### Build

```bash
pio run
```

### Upload

```bash
pio run --target upload
```

### Serial monitor

```bash
pio device monitor
```

The firmware is configured for **Zigbee end-device mode** (`ZIGBEE_MODE_ED`) with a custom partition table (`partitions_zigbee.csv`) that reserves space for Zigbee storage.

## Usage

1. Power on the keypad and wait for it to join your Zigbee network (status dots print over serial at 115200 baud).
2. The status LED glows through the case when the device wakes from deep sleep.
3. Enter a numeric code on the touch pad — each press lights the LED and triggers a short beep.
4. Press **Enter** to transmit the code.
5. Press **Clear** to reset input.
6. After 10 s without a touch, the device enters deep sleep. The next touch wakes it instantly.

### Factory reset

Hold the **BOOT** button for more than 3 seconds. The device performs a Zigbee factory reset and reboots, allowing re-pairing to a new coordinator.

## Project structure

```
keypad/
├── main.cpp                  # Firmware — touch handling, Zigbee, power management
├── platformio.ini            # Board, libraries, and build flags
├── partitions_zigbee.csv     # Flash partition layout for Zigbee
├── case_mid_final.stl        # 3D-printable enclosure mid-section
├── touch_matrix_Y24.zip      # Latest PCB Gerber files (JLCPCB-ready)
├── touch_matrix_Y23.zip      # Previous PCB revision
└── touch_matrix_*.jpg        # Hardware photos
```

## License

This project is dual-licensed:

| Part | License | File |
|------|---------|------|
| Firmware (`main.cpp`, build config) | [MIT License](LICENSE) | `LICENSE` |
| Hardware (PCB Gerbers, STL, design files) | [CERN Open Hardware Licence v2 — Strongly Reciprocal](LICENSE.hardware) | `LICENSE.hardware` |
