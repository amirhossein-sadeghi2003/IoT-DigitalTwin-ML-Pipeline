# IoT-DigitalTwin-ML-Pipeline
ESP32 → MQTT (Mosquitto) → ML Inference → Node-RED Dashboard

## Quick start
- MQTT broker: localhost:1883
- Topic in: `iot/model/input`
- Topic out: `iot/model/predictions`

Run one model server:
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r ml_pipeline/requirements.txt
python ml_pipeline/models/decision_tree/server_tree.py
# or: python ml_pipeline/models/neural_network/server_neural_network.py
```

Node-RED: import `node_red_dashboard/flow.json` and open the dashboard.
ESP32 firmware: `esp32_code/main/main.ino`
