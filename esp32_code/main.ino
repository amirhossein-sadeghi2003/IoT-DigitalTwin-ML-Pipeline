#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_VL53L0X.h>
// replace your ssid and pass word
const char* ssid = "your_ssid";
const char* pass = "your_password";

const char* mqttHost = "broker.hivemq.com";
const uint16_t mqttPort = 1883;
const char* topicPub = "iot/test/sensors";
const char* topicSub = "iot/cmd/arm";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BME280 bme;
BH1750 bh;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

bool ok_bme = false;
bool ok_bh = false;
bool ok_lox = false;

const int PIN_MAG = 18;
unsigned long lastSend = 0;
const unsigned long sendPeriod = 3000;

void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("wifi..");
  WiFi.begin(ssid, pass);
  int c = 0;
  while (WiFi.status() != WL_CONNECTED && c < 40) {
    delay(500);
    Serial.print(".");
    c++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "ok" : "fail");
}

void mqttCallback(char* topic, byte* msg, unsigned int len) {
  Serial.print("[msg] ");
  Serial.print(topic);
  Serial.print(" => ");
  for (unsigned int i = 0; i < len; i++) {
    Serial.print((char)msg[i]);
  }
  Serial.println();
}

void mqttConnect() {
  client.setServer(mqttHost, mqttPort);
  client.setCallback(mqttCallback);
  while (!client.connected()) {
    String cid = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("mqtt..");
    if (client.connect(cid.c_str())) {
      Serial.println("ok");
      client.subscribe(topicSub);
      client.publish(topicPub, "{\"status\":\"online\"}");
    } else {
      Serial.print("rc=");
      Serial.println(client.state());
      delay(800);
    }
  }
}

void initSensors() {
  if (bme.begin(0x76) || bme.begin(0x77)) ok_bme = true;
  else Serial.println("no bme280");

  if (bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) ok_bh = true;
  else Serial.println("no bh1750");

  if (lox.begin()) ok_lox = true;
  else Serial.println("no vl53l0x");

  pinMode(PIN_MAG, INPUT_PULLUP);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  wifiConnect();
  mqttConnect();
  initSensors();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!client.connected()) mqttConnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastSend < sendPeriod) return;
  lastSend = now;

  float t = NAN, h = NAN, p = NAN, lx = NAN;
  int mag = 0;
  int dist = -1;

  if (ok_bme) {
    t = bme.readTemperature();
    h = bme.readHumidity();
    p = bme.readPressure() / 100.0f;
  }
  if (ok_bh) {
    lx = bh.readLightLevel();
  }
  if (ok_lox) {
    VL53L0X_RangingMeasurementData_t m;
    lox.rangingTest(&m, false);
    if (m.RangeStatus == 0) dist = m.RangeMilliMeter;
  }

  mag = (digitalRead(PIN_MAG) == LOW) ? 1 : 0;

  String js = "{";
  js += "\"t\":" + (isnan(t) ? String("null") : String(t,1));
  js += ",\"hum\":" + (isnan(h) ? String("null") : String(h,1));
  js += ",\"pressure\":" + (isnan(p) ? String("null") : String(p,1));
  js += ",\"lux\":" + (isnan(lx) ? String("null") : String(lx,1));
  js += ",\"mag\":" + String(mag);
  js += ",\"dist\":" + (dist < 0 ? String("null") : String(dist));
  js += "}";

  bool s = client.publish(topicPub, js.c_str());
  Serial.print("pub "); Serial.print(s ? "ok: " : "fail: "); Serial.println(js);
}

