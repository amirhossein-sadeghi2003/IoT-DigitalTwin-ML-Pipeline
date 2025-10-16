import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report
import pickle
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

rng = np.random.default_rng(42)
n_samples = 50000 


temp = rng.normal(25, 5, n_samples)
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
    'temp': temp,
    'humidity': humidity,
    'pressure': pressure,
    'light': light,
    'magnet': magnet,
    'distance': distance,
    'alarm': alarm,
    'distance_missing': distance_missing,
    'heater': heater,
    'cooler': cooler,
    'flood': flood,
    'window': window
})

X = data[['temp','humidity','pressure','light','magnet','distance','alarm','distance_missing']].values
y = data[['heater','cooler','flood','window']].values

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=alarm
)

scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

with open('scaler.pkl', 'wb') as f:
    pickle.dump(scaler, f)


inputs = keras.Input(shape=(X_train_scaled.shape[1],))
x = layers.Dense(64, activation='relu')(inputs)
x = layers.Dropout(0.1)(x) 
x = layers.Dense(32, activation='relu')(x)
x = layers.Dropout(0.1)(x)
x = layers.Dense(16, activation='relu')(x) 
outputs = layers.Dense(4, activation='sigmoid')(x)

model = keras.Model(inputs, outputs, name='iot_multioutput_mlp_focused')
model.compile(
    optimizer=keras.optimizers.Adam(learning_rate=1e-3),
    loss='binary_crossentropy',
    metrics=[keras.metrics.BinaryAccuracy(name='bin_acc'), keras.metrics.AUC(name='auc')]
)

callbacks = [
    keras.callbacks.EarlyStopping(monitor='val_loss', patience=8, restore_best_weights=True)
]

history = model.fit(
    X_train_scaled, y_train.astype('float32'),
    validation_split=0.2,
    epochs=100,
    batch_size=256,
    callbacks=callbacks,
    verbose=1
)

y_pred_prob = model.predict(X_test_scaled)
y_pred = (y_pred_prob >= 0.5).astype(int)

target_names = ['heater','cooler','flood','window']
print("=== Classification Report (per actuator) ===")
print(classification_report(y_test, y_pred, target_names=target_names, digits=4))

model.save('iot_multioutput_mlp.keras')
