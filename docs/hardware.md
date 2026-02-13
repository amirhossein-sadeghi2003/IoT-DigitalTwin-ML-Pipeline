# Hardware / Wiring

## Parts
- ESP32 DevKit
- (Sensor) Temperature
- (Sensor) Light (LDR)
- (Sensor) Magnetic reed switch (door/window)
- (Optional) LEDs / relays for outputs (heater/cooler/flood/window indicators)
- Jumper wires + breadboard

## Wiring (high level)
> Update the GPIO numbers based on your actual wiring / code.

- **Temperature sensor** → ESP32 GPIO: ___
- **LDR (light)** → ESP32 ADC GPIO: ___
- **Reed switch (magnet)** → ESP32 GPIO: ___ (use pull-up/down as needed)
- **Alarm input (if any)** → ESP32 GPIO: ___

## Notes
- Ensure all grounds are common (GND).
- If using relays, use proper drivers and power isolation.
- Keep MQTT broker settings in `esp32_code/config.h` (ignored by git).
