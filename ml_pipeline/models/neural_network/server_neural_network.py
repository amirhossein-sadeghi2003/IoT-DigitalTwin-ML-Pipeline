import numpy as np
import json
import time
import pickle
import paho.mqtt.client as mqtt
from tensorflow import keras

try:
    model = keras.models.load_model('iot_multioutput_mlp.keras')
except Exception as e:
    print("Error: could not load 'iot_multioutput_mlp.keras'. Train/export the NN model first.")
    raise

try:
    with open('scaler.pkl', 'rb') as f:
        scaler = pickle.load(f)
except FileNotFoundError:
    print("Error: 'scaler.pkl' not found. Train/export the scaler first.")
    raise

broker = "localhost"
port = 1883
topic_input = "iot/model/input"
topic_output = "iot/model/predictions"
QOS = 1

FEATURES = ['temp','humidity','pressure','light','magnet','distance','alarm','distance_missing']
TARGETS = ['heater','cooler','flood','window']

def to_float_or_default(v, default=0.0):
    if v is None:
        return default
    if isinstance(v, str):
        s = v.strip().lower()
        if s == 'null' or s == '':
            return default
        try:
            return float(s)
        except:
            return default
    try:
        return float(v)
    except:
        return default

def build_input_vector(data: dict):
    raw_distance = data.get('distance', None)
    dist_missing = (raw_distance is None) \
                   or (isinstance(raw_distance, str) and raw_distance.strip().lower() == 'null')

    if not dist_missing and isinstance(raw_distance, (int, float)) and float(raw_distance) == -1.0:
        dist_missing = True

    distance_value = -1.0 if dist_missing else to_float_or_default(raw_distance, default=-1.0)
    distance_missing_flag = 1.0 if dist_missing else 0.0

    vals = {
        'temp': to_float_or_default(data.get('temp'), 0.0),
        'humidity': to_float_or_default(data.get('humidity'), 0.0),
        'pressure': to_float_or_default(data.get('pressure'), 0.0),
        'light': to_float_or_default(data.get('light'), 0.0),
        'magnet': to_float_or_default(data.get('magnet'), 0.0),
        'distance': distance_value,
        'alarm': to_float_or_default(data.get('alarm'), 0.0),
        'distance_missing': distance_missing_flag
    }

    x = [vals[f] for f in FEATURES]
    return np.array(x, dtype=np.float32), int(round(vals['alarm']))

def predict_commands(x_vec: np.ndarray, alarm_flag: int, threshold=0.5, enforce_alarm_rule=True):
    x_scaled = scaler.transform(x_vec.reshape(1, -1))

    probs = model.predict(x_scaled, verbose=0)[0]
    preds = (probs >= threshold).astype(int)

    if enforce_alarm_rule and alarm_flag == 1:
        preds = np.array([0, 0, 0, 0], dtype=int)

    messages = [f"{t}={int(v)}" for t, v in zip(TARGETS, preds)]
    return messages, probs.tolist()

def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("Connected to MQTT Broker.")
        client.subscribe(topic_input, qos=QOS)
        print(f"Subscribed: {topic_input} (QoS {QOS})")
    else:
        print(f"Failed to connect, code: {reason_code}")

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        print(f"Received: {data}")

        x_vec, alarm_flag = build_input_vector(data)
        messages, probs = predict_commands(x_vec, alarm_flag, threshold=0.5, enforce_alarm_rule=True)

        for m in messages:
            client.publish(topic_output, m, qos=QOS, retain=False)
            print(f"Published: {m} -> {topic_output}")

        client.publish(topic_output, json.dumps({"probs": dict(zip(TARGETS, probs))}), qos=QOS, retain=False)

    except json.JSONDecodeError:
        print(f"JSON decode error. Payload: {msg.payload.decode(errors='ignore')}")
    except Exception as e:
        print(f"Error during processing: {e}")

if __name__ == "__main__":
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(broker, port, 60)
        client.loop_start()
        print("NN Model Service is running. Press Ctrl+C to stop.")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nService stopped by user.")
    except Exception as e:
        print(f"Fatal error: {e}")
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected.")
