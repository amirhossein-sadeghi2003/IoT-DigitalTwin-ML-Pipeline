#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

const char* ssid = "YOUR_SSID";
const char* pass = "YOUR_PASS";

const char* mqttHost = "broker.hivemq.com";
const uint16_t mqttPort = 1883;
const char* topicPub = "iot/test/sensors";
const char* topicSub = "iot/cmd/arm";

WiFiClient net;
PubSubClient mqtt(net);

Adafruit_BME280 bme;
BH1750 luxm;

bool hasBme = false;
bool hasLux = false;

const int MAG_PIN = 18;
unsigned long lastTick = 0;
const unsigned long period = 3000;

static void wifiUp() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("wifi..");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  int k = 0;
  while (WiFi.status() != WL_CONNECTED && k++ < 40) {
    delay(500); Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "ok" : "fail");
}

static void onMqtt(char* topic, byte* payload, unsigned int n) {
  Serial.print("[cmd] "); Serial.print(topic); Serial.print(" => ");
  for (unsigned int i = 0; i < n; ++i) Serial.print((char)payload[i]);
  Serial.println();
}

static void mqttUp() {
  mqtt.setServer(mqttHost, mqttPort);
  mqtt.setCallback(onMqtt);
  while (!mqtt.connected()) {
    String cid = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("mqtt..");
    if (mqtt.connect(cid.c_str())) {
      Serial.println("ok");
      mqtt.subscribe(topicSub);
      mqtt.publish(topicPub, "{\"status\":\"online\"}");
    } else {
      Serial.print("rc="); Serial.println(mqtt.state());
      delay(800);
    }
  }
}

static void initSensors() {
  if (bme.begin(0x76) || bme.begin(0x77)) hasBme = true;
  else Serial.println("no bme280");

  if (luxm.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) hasLux = true;
  else Serial.println("no bh1750");

  pinMode(MAG_PIN, INPUT_PULLUP);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  wifiUp();
  mqttUp();
  initSensors();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) wifiUp();
  if (!mqtt.connected()) mqttUp();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastTick < period) return;
  lastTick = now;

  float t = NAN, h = NAN, p = NAN, lx = NAN;
  if (hasBme) {
    t = bme.readTemperature();
    h = bme.readHumidity();
    p = bme.readPressure() / 100.0f;
  }
  if (hasLux) {
    lx = luxm.readLightLevel();
  }
  int mag = (digitalRead(MAG_PIN) == LOW) ? 1 : 0;

  String js = String("{\"t\":") + (isnan(t)? "null": String(t,1)) +
              ",\"hum\":" + (isnan(h)? "null": String(h,1)) +
              ",\"pressure\":" + (isnan(p)? "null": String(p,1)) +
              ",\"lux\":" + (isnan(lx)? "null": String(lx,1)) +
              ",\"mag\":" + mag + "}";

  bool ok = mqtt.publish(topicPub, js.c_str());
  Serial.print("pub "); Serial.print(ok? "ok: " : "fail: "); Serial.println(js);
}

