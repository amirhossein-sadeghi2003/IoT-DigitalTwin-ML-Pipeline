# Node-RED Dashboard

## Import

1. Open the Node-RED editor
2. Menu → Import → select `flow.json`
3. Deploy

## MQTT

- Sensor input: `iot/test/sensors`
- Model input: `iot/model/input`
- Model predictions: `iot/model/predictions`
- Actuator commands: `iot/cmd/act`

## Notes

The included flow uses `localhost:1883` by default. Update the broker configuration if a different host or port is used.
