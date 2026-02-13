# Hardware / Wiring

## Parts
- ESP32 DevKit
- (Sensor) Magnetic reed switch (door/window)
- (Optional) LED indicator
- (Optional) Buzzer
- (Sensor) Temperature (TBD from code)
- (Sensor) Light / LDR (TBD from code)
- Jumper wires + breadboard

## GPIO mapping (from `esp32_code/main/main.ino`)
- **LED** → GPIO **5** (`LED_PIN`)
- **Buzzer** → GPIO **23** (`BUZZER_PIN`)
- **Reed switch (magnet/door)** → GPIO **18** (`magnet_pin`)
  - Config: `INPUT_PULLUP`
  - Wiring: one side to **GND**, the other side to **GPIO18**
  - Logic in code: `LOW` means **magnet = 1**

## Wiring (high level)
- Keep **all grounds common (GND)**.
- If you use relays (heater/cooler/etc), use proper driver/transistor + separate power.

## TODO (to fill once confirmed from code)
- Temperature sensor GPIO: ___
- LDR/Light ADC GPIO: ___
