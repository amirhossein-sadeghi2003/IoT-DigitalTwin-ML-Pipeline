import json
import pickle
import time
from pathlib import Path

import pandas as pd
import paho.mqtt.client as mqtt


ARTIFACT_DIR = Path(__file__).resolve().parents[1]
MODEL_PATH = ARTIFACT_DIR / "decision_tree_model.pkl"

try:
    with MODEL_PATH.open("rb") as model_file:
        model = pickle.load(model_file)
except FileNotFoundError:
    print("Error: 'decision_tree_model.pkl' not found.")
    raise SystemExit


broker = "localhost"
port = 1883
topic_input = "iot/model/input"
topic_output = "iot/model/predictions"
qos = 1

feature_cols = [
    "temp",
    "humidity",
    "pressure",
    "light",
    "magnet",
    "distance",
    "alarm",
    "distance_missing",
]

target_cols = ["heater", "cooler", "flood", "window"]


def to_float(x, default=0.0):
    if x is None:
        return float(default)

    if isinstance(x, str) and x.lower() == "null":
        return float(default)

    try:
        return float(x)
    except (TypeError, ValueError):
        return float(default)


def process_message(data):
    vals = {}

    for key in [
        "temp",
        "humidity",
        "pressure",
        "light",
        "magnet",
        "distance",
        "alarm",
    ]:
        value = data.get(key)

        if key == "distance":
            if value is None or (
                isinstance(value, str) and value.lower() == "null"
            ):
                vals["distance"] = -1.0
                vals["distance_missing"] = 1.0
            else:
                distance = to_float(value)
                vals["distance"] = distance
                vals["distance_missing"] = 1.0 if distance == -1 else 0.0
        else:
            vals[key] = to_float(value)

    row = {column: vals.get(column, 0.0) for column in feature_cols}
    df = pd.DataFrame([row], columns=feature_cols)

    prediction = model.predict(df)[0]

    return [
        f"{target}={int(value)}"
        for target, value in zip(target_cols, prediction)
    ]


def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("Connected.")
        client.subscribe(topic_input, qos=qos)
        print("Subscribed:", topic_input)
    else:
        print("Connect failed:", reason_code)


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        print("Received:", data)

        messages = process_message(data)

        for message in messages:
            client.publish(
                topic_output,
                message,
                qos=qos,
                retain=False,
            )
            print("Published:", message, "->", topic_output)

    except json.JSONDecodeError:
        print("Bad JSON:", msg.payload.decode())
    except Exception as exc:
        print("Processing error:", exc)


def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(broker, port, 60)
        client.loop_start()

        print("AI Model Service is running. Press Ctrl+C to stop.")

        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("Stopped by user.")
    except Exception as exc:
        print("Runtime error:", exc)
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected.")


if __name__ == "__main__":
    main()
