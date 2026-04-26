# IoT Digital Twin ML Pipeline

This project is an IoT-based digital twin system that combines embedded sensing, MQTT communication, machine learning inference, and a Node-RED dashboard for monitoring and control.

## Overview

The system is built around an ESP32-based sensing unit that collects environmental data and sends it through MQTT. The received data is then processed by machine learning models, and the predictions are visualized in a Node-RED dashboard.

The full pipeline is:

`ESP32 -> MQTT (Mosquitto) -> ML Inference -> Node-RED Dashboard`

## Main Components

### 1. ESP32 Sensing Layer

The ESP32 collects real-world sensor data from:

- BME280 for temperature, humidity, and pressure
- BH1750 for ambient light

These sensors communicate over I2C.

### 2. MQTT Communication Layer

The sensor data is transmitted using MQTT.

Main topics:

- input topic: `iot/model/input`
- output topic: `iot/model/predictions`

Default broker:

- `localhost:1883`

### 3. Machine Learning Layer

The project includes machine learning models for prediction and intelligent system behavior.

The ML pipeline is located in the `ml_pipeline` folder and supports different model types such as decision trees and neural networks.

### 4. Visualization Layer

The system uses Node-RED to visualize the incoming data and model predictions in a dashboard interface.

The dashboard flow file is included in:

`node_red_dashboard/flow.json`

## Repository Structure

Main parts of the repository:

- `esp32_code/` for ESP32 firmware
- `ml_pipeline/` for model training and inference
- `node_red_dashboard/` for Node-RED flow and dashboard setup
- `docs/` for documentation, images, and presentation

## Quick Start

To run the ML environment:

- create and activate a virtual environment
- install the required packages from `ml_pipeline/requirements.txt`
- run one of the model servers

The ESP32 firmware entry point is:

`esp32_code/main/main.ino`

The Node-RED dashboard flow is:

`node_red_dashboard/flow.json`

## Documentation

Additional documentation is available in:

- `docs/hardware.md`
- `docs/mqtt_examples.md`
- `node_red_dashboard/README.md`

## Screenshots

### Dashboard

The dashboard interface is shown in:

`docs/images/dashboard.png`

### Hardware and Circuit

The hardware setup is shown in:

`docs/images/circuit.jpeg`

## Project Presentation

The English project presentation is available here:

`docs/presentation.pdf`

## Hardware Note

Sensors used in this project:

- BME280
- BH1750

ESP32 default I2C pins:

- SDA = 21
- SCL = 22

## Project Goal

The main goal of this project is to create an intelligent digital twin pipeline for environmental monitoring and decision support by combining:

- embedded systems
- IoT communication
- machine learning
- dashboard-based visualization

## Future Improvements

Possible next steps:

- improve deployment structure
- add more robust model serving
- connect the system to real-time cloud infrastructure
- improve model evaluation and logging
- extend the dashboard for richer control features
