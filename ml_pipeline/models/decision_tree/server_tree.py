import numpy as np
import pandas as pd
import json
import pickle
import time
import paho.mqtt.client as mqtt

try:
    with open("decision_tree_model.pkl", "rb") as f:
        model = pickle.load(f)
except FileNotFoundError:
    print("Error: 'decision_tree_model.pkl' not found.")
    raise SystemExit

broker = "localhost"
port = 1883
topic_input = "iot/model/input"
topic_output = "iot/model/predictions"
qos = 1

feature_cols = ["temp", "humidity", "pressure", "light", "magnet", "distance", "alarm", "distance_missing"]
target_cols = ["heater", "cooler", "flood", "window"]

def to_float(x, default=0.0):
    if x is None:
        return float(default)
    if isinstance(x, str) and x.lower() == "null":
        return float(default)
    try:
        return float(x)
    except:
        return float(default)

def process_message(data):
    vals = {}
    for k in ["temp", "humidity", "pressure", "light", "magnet", "distance", "alarm"]:
        v = data.get(k)
        if k == "distance":
            if v is None or (isinstance(v, str) and v.lower() == "null"):
                vals["distance"] = -1.0
                vals["distance_missing"] = 1.0
            else:
                d = to_float(v)
                vals["distance"] = d
                vals["distance_missing"] = 1.0 if d == -1 else 0.0
        else:
            vals[k] = to_float(v)
    row = {c: vals.get(c, 0.0) for c in feature_cols}
    df = pd.DataFrame([row], columns=feature_cols)
    pred = model.predict(df)[0]
    out = [f"{t}={int(p)}" for t, p in zip(target_cols, pred)]
    return out

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
        for m in messages:
            client.publish(topic_output, m, qos=qos, retain=False)
            print("Published:", m, "->", topic_output)
    except json.JSONDecodeError:
        print("Bad JSON:", msg.payload.decode())
    except Exception as e:
        print("Processing error:", e)

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
except Exception as e:
    print("Runtime error:", e)
finally:
    client.loop_stop()
    client.disconnect()
    print("Disconnected.")

