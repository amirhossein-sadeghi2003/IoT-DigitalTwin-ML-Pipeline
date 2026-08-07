import pickle
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.metrics import accuracy_score, confusion_matrix
from sklearn.model_selection import train_test_split
from sklearn.multioutput import MultiOutputClassifier
from sklearn.tree import DecisionTreeClassifier


ARTIFACT_DIR = Path(__file__).resolve().parents[1]
MODEL_PATH = ARTIFACT_DIR / "decision_tree_model.pkl"

N_SAMPLES = 10000

FEATURE_COLS = [
    "temp",
    "humidity",
    "pressure",
    "light",
    "magnet",
    "distance",
    "alarm",
    "distance_missing",
]

TARGET_COLS = [
    "heater",
    "cooler",
    "flood",
    "window",
]


def main():
    rng = np.random.default_rng(42)

    temp = rng.normal(30, 5, N_SAMPLES)
    humidity = rng.normal(20, 5, N_SAMPLES)
    pressure = rng.normal(843, 10, N_SAMPLES)
    light = rng.normal(100, 10, N_SAMPLES)
    magnet = rng.integers(0, 2, N_SAMPLES)
    distance = rng.normal(100, 50, N_SAMPLES)

    distance_missing = rng.choice(
        [0, 1],
        N_SAMPLES,
        p=[0.8, 0.2],
    )
    distance = np.where(distance_missing == 1, -1, distance)

    alarm = rng.integers(0, 2, N_SAMPLES)

    heater = (temp < 20).astype(int)
    cooler = (temp > 28).astype(int)
    flood = (light < 100).astype(int)
    window = (magnet == 0).astype(int)

    mask_alarm = alarm == 1
    heater[mask_alarm] = 0
    cooler[mask_alarm] = 0
    flood[mask_alarm] = 0
    window[mask_alarm] = 0

    data = pd.DataFrame(
        {
            "temp": temp,
            "humidity": humidity,
            "pressure": pressure,
            "light": light,
            "magnet": magnet,
            "distance": distance,
            "alarm": alarm,
            "distance_missing": distance_missing,
            "heater": heater,
            "cooler": cooler,
            "flood": flood,
            "window": window,
        }
    )

    X = data[FEATURE_COLS]
    y = data[TARGET_COLS]

    X_train, X_test, y_train, y_test = train_test_split(
        X,
        y,
        test_size=0.2,
        random_state=7,
        shuffle=True,
    )

    model = MultiOutputClassifier(
        DecisionTreeClassifier(
            max_depth=5,
            random_state=42,
        )
    )
    model.fit(X_train, y_train)

    with MODEL_PATH.open("wb") as model_file:
        pickle.dump(model, model_file)

    y_pred = model.predict(X_test)

    print(
        "Exact match accuracy:",
        accuracy_score(y_test.values, y_pred),
    )

    for index, target in enumerate(TARGET_COLS):
        accuracy = accuracy_score(
            y_test[target].values,
            y_pred[:, index],
        )

        print(target, "accuracy:", accuracy)
        print(
            confusion_matrix(
                y_test[target].values,
                y_pred[:, index],
            )
        )

    X_alarm = X_test.copy()
    X_alarm["alarm"] = 1

    y_alarm_pred = model.predict(X_alarm)
    all_zero_rate = (y_alarm_pred.sum(axis=1) == 0).mean()

    print("Alarm=1 all-zeros rate:", all_zero_rate)


if __name__ == "__main__":
    main()
