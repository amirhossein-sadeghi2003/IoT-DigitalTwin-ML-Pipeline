# Hardware / Wiring

## Parts
- ESP32 DevKit
- **BME280** (temperature / humidity / pressure) — I2C
- **BH1750** (light sensor) — I2C
- Magnetic reed switch (door/window)
- (Optional) LED indicator
- (Optional) Buzzer
- Jumper wires + breadboard

## GPIO mapping (from `esp32_code/main/main.ino`)
- **LED** → GPIO **5** (`LED_PIN`)
- **Buzzer** → GPIO **23** (`BUZZER_PIN`)
- **Reed switch (magnet/door)** → GPIO **18** (`magnet_pin`)
  - Config: `INPUT_PULLUP`
  - Wiring: one side to **GND**, the other side to **GPIO18**
  - Logic in code: `LOW` means **magnet = 1**

## I2C sensors
The code uses `Wire.begin()` (default ESP32 I2C pins).
- **I2C SDA**: GPIO **21** (common default)
- **I2C SCL**: GPIO **22** (common default)

### BME280
- Address: `0x76` or `0x77` (auto-checked in code)
- Pins: VCC, GND, SDA, SCL

### BH1750
- Runs in `CONTINUOUS_HIGH_RES_MODE`
- Pins: VCC, GND, SDA, SCL

## Wiring notes
- Keep **all grounds common (GND)**.
- If you use relays (heater/cooler/etc), use proper driver/transistor + separate power.
