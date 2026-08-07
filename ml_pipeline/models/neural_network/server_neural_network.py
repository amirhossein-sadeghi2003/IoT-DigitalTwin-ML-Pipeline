import json
import pickle
import time
from pathlib import Path

import numpy as np
import paho.mqtt.client as mqtt
from tensorflow import keras


ARTIFACT_DIR = Path(__file__).resolve().parents[1]
MODEL_PATH = ARTIFACT_DIR / "iot_multioutput_mlp.keras"
SCALER_PATH = ARTIFACT_DIR / "scaler.pkl"

BROKER = "localhost"
PORT = 1883
TOPIC_INPUT = "iot/model/input"
TOPIC_OUTPUT = "iot/model/predictions"
QOS = 1

FEATURES = [
    "temp",
    "humidity",
    "pressure",
    "light",
    "magnet",
    "distance",
    "alarm",
    "distance_missing",
]

TARGETS = [
    "heater",
    "cooler",
    "flood",
    "window",
]


try:
    model = keras.models.load_model(MODEL_PATH)
except Exception:
    print(
        "Error: could not load 'iot_multioutput_mlp.keras'. "
        "Train/export the NN model first."
    )
    raise


try:
    with SCALER_PATH.open("rb") as scaler_file:
        scaler = pickle.load(scaler_file)
except FileNotFoundError:
    print(
        "Error: 'scaler.pkl' not found. "
        "Train/export the scaler first."
    )
    raise


def to_float_or_default(value, default=0.0):
    if value is None:
        return default

    if isinstance(value, str):
        normalized = value.strip().lower()

        if normalized == "null" or normalized == "":
            return default

        try:
            return float(normalized)
        except (TypeError, ValueError):
            return default

    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def build_input_vector(data: dict):
    raw_distance = data.get("distance", None)

    distance_missing = (
        raw_distance is None
        or (
            isinstance(raw_distance, str)
            and raw_distance.strip().lower() == "null"
        )
    )

    if (
        not distance_missing
        and isinstance(raw_distance, (int, float))
        and float(raw_distance) == -1.0
    ):
        distance_missing = True

    distance_value = (
        -1.0
        if distance_missing
        else to_float_or_default(raw_distance, default=-1.0)
    )
    distance_missing_flag = 1.0 if distance_missing else 0.0

    values = {
        "temp": to_float_or_default(data.get("temp"), 0.0),
        "humidity": to_float_or_default(data.get("humidity"), 0.0),
        "pressure": to_float_or_default(data.get("pressure"), 0.0),
        "light": to_float_or_default(data.get("light"), 0.0),
        "magnet": to_float_or_default(data.get("magnet"), 0.0),
        "distance": distance_value,
        "alarm": to_float_or_default(data.get("alarm"), 0.0),
        "distance_missing": distance_missing_flag,
    }

    input_values = [values[feature] for feature in FEATURES]

    return (
        np.array(input_values, dtype=np.float32),
        int(round(values["alarm"])),
    )


def predict_commands(
    input_vector: np.ndarray,
    alarm_flag: int,
    threshold=0.5,
    enforce_alarm_rule=True,
):
    input_scaled = scaler.transform(input_vector.reshape(1, -1))

    probabilities = model.predict(input_scaled, verbose=0)[0]
    predictions = (probabilities >= threshold).astype(int)

    if enforce_alarm_rule and alarm_flag == 1:
        predictions = np.array([0, 0, 0, 0], dtype=int)

    messages = [
        f"{target}={int(value)}"
        for target, value in zip(TARGETS, predictions)
    ]

    return messages, probabilities.tolist()


def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("Connected to MQTT Broker.")
        client.subscribe(TOPIC_INPUT, qos=QOS)
        print(f"Subscribed: {TOPIC_INPUT} (QoS {QOS})")
    else:
        print(f"Failed to connect, code: {reason_code}")


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        print(f"Received: {data}")

        input_vector, alarm_flag = build_input_vector(data)

        messages, probabilities = predict_commands(
            input_vector,
            alarm_flag,
            threshold=0.5,
            enforce_alarm_rule=True,
        )

        for message in messages:
            client.publish(
                TOPIC_OUTPUT,
                message,
                qos=QOS,
                retain=False,
            )
            print(f"Published: {message} -> {TOPIC_OUTPUT}")

        probability_payload = {
            "probs": dict(zip(TARGETS, probabilities))
        }

        client.publish(
            TOPIC_OUTPUT,
            json.dumps(probability_payload),
            qos=QOS,
            retain=False,
        )

    except json.JSONDecodeError:
        print(
            "JSON decode error. Payload: "
            f"{msg.payload.decode(errors='ignore')}"
        )
    except Exception as exc:
        print(f"Error during processing: {exc}")


def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER, PORT, 60)
        client.loop_start()

        print("NN Model Service is running. Press Ctrl+C to stop.")

        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\nService stopped by user.")
    except Exception as exc:
        print(f"Fatal error: {exc}")
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected.")


if __name__ == "__main__":
    main()
