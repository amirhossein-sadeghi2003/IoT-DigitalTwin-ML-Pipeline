#include <WiFi.h>
#include <PubSubClient.h>
//replace your wifi ssid and password
const char* ssid     = "your_ssid";
const char* password = "your_password";

const char* mqttHost = "broker.hivemq.com";
const uint16_t mqttPort = 1883;
const char* topicPub = "iot/test/heartbeat";
const char* topicSub = "iot/cmd/arm";

WiFiClient net;
PubSubClient mqtt(net);

unsigned long lastBeat = 0;
const unsigned long beatInterval = 3000;

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("WiFi...");
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
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("[MQTT] ");
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

    Serial.print("MQTT...");
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

void setup() {
  Serial.begin(115200);
  delay(200);
  connectWifi();
  connectMqtt();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWifi();
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastBeat >= beatInterval) {
    lastBeat = now;
    long rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

    String payload = String("{\"ms\":") + now + ",\"rssi\":" + rssi + "}";
    bool ok = mqtt.publish(topicPub, payload.c_str());

    Serial.print("publish ");
    Serial.print(ok ? "ok: " : "fail: ");
    Serial.println(payload);
  }
}


