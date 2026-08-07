# MQTT Examples

## Topics

Model input: `iot/model/input`

Model output: `iot/model/predictions`

## Example Input

Example JSON message published to the model input topic:

    {
      "temp": 25.0,
      "humidity": 40.0,
      "pressure": 843.0,
      "light": 200.0,
      "magnet": 1,
      "distance": 120,
      "alarm": 0
    }

If the distance sensor does not provide a valid reading, `distance` can be sent as `null`.

## Example Output

Model servers publish actuator decisions to the prediction topic using messages such as:

    heater=0
    cooler=1
    flood=0
    window=0
The neural-network service can also publish a JSON message containing prediction probabilities.
