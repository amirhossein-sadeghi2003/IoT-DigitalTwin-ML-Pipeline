# Machine Learning Pipeline

This folder contains the software-side decision layer used by the IoT digital twin prototype.

The hardware prototype publishes sensor readings through MQTT, but the decision logic can also be tested offline. This makes the software layer easier to check without rebuilding the ESP32 circuit or running the full Node-RED dashboard.

## Current Inputs

The decision layer uses these telemetry fields:

- temp
- humidity
- pressure
- light
- magnet
- distance
- alarm

The replay script also derives `distance_missing` when the distance field is empty, null-like, or -1.

## Current Outputs

The current decision outputs are:

- heater
- cooler
- flood
- window

These represent simple actuator or state decisions used by the digital twin dashboard.

## Offline Replay

A small sample telemetry file is included here:

- ml_pipeline/data/sample_telemetry.csv

Run the replay script from the repository root:

    python ml_pipeline/scripts/replay_decision_logic.py

Generated outputs:

- ml_pipeline/results/decision_replay_predictions.csv
- ml_pipeline/results/decision_replay_summary.txt

This replay does not require MQTT, Node-RED, or the ESP32 hardware. It is meant to make the decision layer reproducible from the repository alone.

## What the Replay Checks

The sample scenarios include:

- normal room conditions
- low temperature
- high temperature
- low light
- open window or contact state
- multiple simultaneous conditions
- missing distance reading
- alarm override behavior

The alarm override case is important because the current logic forces all actuator outputs off when alarm equals 1.

## Scope

This is still a prototype decision layer. The replay is not a trained production model evaluation. Its purpose is to make the software behavior visible and testable without the physical IoT setup.
