import numpy as np
import pandas as pd
from sklearn.tree import DecisionTreeClassifier
from sklearn.multioutput import MultiOutputClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, confusion_matrix
import pickle

n_samples = 10000
rng = np.random.default_rng(42)

temp = rng.normal(30, 5, n_samples)
humidity = rng.normal(20, 5, n_samples)
pressure = rng.normal(843, 10, n_samples)
light = rng.normal(100, 10, n_samples)
magnet = rng.integers(0, 2, n_samples)
distance = rng.normal(100, 50, n_samples)
distance_missing = rng.choice([0, 1], n_samples, p=[0.8, 0.2])
distance = np.where(distance_missing == 1, -1, distance)
alarm = rng.integers(0, 2, n_samples)

heater = (temp < 20).astype(int)
cooler = (temp > 28).astype(int)
flood = (light < 100).astype(int)
window = (magnet == 0).astype(int)

mask_alarm = (alarm == 1)
heater[mask_alarm] = 0
cooler[mask_alarm] = 0
flood[mask_alarm] = 0
window[mask_alarm] = 0

data = pd.DataFrame({
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
    "window": window
})

X = data[["temp", "humidity", "pressure", "light", "magnet", "distance", "alarm", "distance_missing"]]
y = data[["heater", "cooler", "flood", "window"]]

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=7, shuffle=True)

model = MultiOutputClassifier(DecisionTreeClassifier(max_depth=5, random_state=42))
model.fit(X_train, y_train)

with open("decision_tree_model.pkl", "wb") as f:
    pickle.dump(model, f)

y_pred = model.predict(X_test)
print("Exact match accuracy:", accuracy_score(y_test.values, y_pred))

for i, col in enumerate(["heater", "cooler", "flood", "window"]):
    acc = accuracy_score(y_test[col].values, y_pred[:, i])
    print(col, "accuracy:", acc)
    print(confusion_matrix(y_test[col].values, y_pred[:, i]))

X_alarm = X_test.copy()
X_alarm["alarm"] = 1
y_alarm_pred = model.predict(X_alarm)
all_zero_rate = (y_alarm_pred.sum(axis=1) == 0).mean()
print("Alarm=1 all-zeros rate:", all_zero_rate)

