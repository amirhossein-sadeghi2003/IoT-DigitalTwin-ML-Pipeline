#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//replace your wifi ssid and password
const char* ssid     = "your-ssid";
const char* password = "your_password";

const char* mqttHost = "broker.hivemq.com";
const uint16_t mqttPort = 1883;
const char* topicPub = "iot/test/heartbeat";
const char* topicSub = "iot/cmd/arm";

WiFiClient net;
PubSubClient mqtt(net);

Adafruit_BME280 bme;
bool hasBme = false;

unsigned long lastBeat = 0;
const unsigned long beatInterval = 3000;

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("wifi..");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("ip: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("wifi failed");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("[mqtt] ");
  Serial.print(topic);
  Serial.print(" => ");
  for (unsigned int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void connectMqtt() {
  mqtt.setServer(mqttHost, mqttPort);
  mqtt.setCallback(mqttCallback);

  while (!mqtt.connected()) {
    String cid = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("mqtt..");
    if (mqtt.connect(cid.c_str())) {
      Serial.println("ok");
      mqtt.subscribe(topicSub);
      mqtt.publish(topicPub, "{\"status\":\"online\"}");
    } else {
      Serial.print("fail rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void initBme() {
  if (bme.begin(0x76) || bme.begin(0x77)) {
    hasBme = true;
  } else {
    Serial.println("no bme280 found");
    hasBme = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin();
  connectWifi();
  connectMqtt();
  initBme();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWifi();
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastBeat >= beatInterval) {
    lastBeat = now;

    long rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

    float t = NAN, h = NAN, p = NAN;
    if (hasBme) {
      t = bme.readTemperature();
      h = bme.readHumidity();
      p = bme.readPressure() / 100.0f;
    }

    String payload = String("{\"ms\":") + now +
                     ",\"rssi\":" + rssi +
                     ",\"t\":" + (isnan(t) ? String("null") : String(t,1)) +
                     ",\"h\":" + (isnan(h) ? String("null") : String(h,1)) +
                     ",\"p\":" + (isnan(p) ? String("null") : String(p,1)) +
                     "}";

    bool ok = mqtt.publish(topicPub, payload.c_str());
    Serial.print("pub ");
    Serial.print(ok ? "ok: " : "fail: ");
    Serial.println(payload);
  }
}

