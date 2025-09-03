#include <WiFi.h>

// replace with your WiFi credentials
const char* ssid = "your_ssid";
const char* password = "your_password";

unsigned long lastTick = 0;
const unsigned long interval = 1000;

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

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
    Serial.print("Connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect, check WiFi settings");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  connectWifi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  unsigned long now = millis();
  if (now - lastTick >= interval) {
    lastTick = now;
    long rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

    Serial.print("Heartbeat ");
    Serial.print(now);
    Serial.print(" ms  RSSI=");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
}

