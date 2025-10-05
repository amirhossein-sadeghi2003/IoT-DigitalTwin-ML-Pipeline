#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "Mi 11 Lite";
const char* pass = "12345678";

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
bool ok_oled = false;

const int PIN_MAG = 18;
unsigned long lastSend = 0;
const unsigned long sendPeriod = 3000;

uint8_t curPage = 0;
unsigned long lastPageSwitch = 0;
const unsigned long pagePeriod = 5000;

float g_t = NAN, g_h = NAN, g_p = NAN, g_lx = NAN;
int   g_mag = 0, g_dist = -1;

void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(ssid, pass);
  int c = 0;
  while (WiFi.status() != WL_CONNECTED && c < 40) {
    delay(500);
    c++;
  }
}

void mqttCallback(char* topic, byte* msg, unsigned int len) {
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
    if (client.connect(cid.c_str())) {
      client.subscribe(topicSub);
      client.publish(topicPub, "{\"status\":\"online\"}");
    } else {
      delay(800);
    }
  }
}

void initSensors() {
  if (bme.begin(0x76) || bme.begin(0x77)) ok_bme = true;
  if (bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) ok_bh = true;
  if (lox.begin()) ok_lox = true;
  pinMode(PIN_MAG, INPUT_PULLUP);
}

void printLine(int16_t y, const char* label, const String &val) {
  display.setCursor(0, y);
  display.print(label);
  display.print(": ");
  display.print(val);
}

String f1(float x) {
  if (isnan(x)) return String("--");
  return String(x, 1);
}

String i1(int x) {
  if (x < 0) return String("--");
  return String(x);
}

void drawOLED() {
  if (!ok_oled) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (curPage == 0) {
    display.setCursor(0, 0);
    display.println("Sensors (1/2)");
    printLine(16, "T(C)",  f1(g_t));
    printLine(28, "Hum(%)", f1(g_h));
    printLine(40, "P(hPa)", f1(g_p));
    display.setCursor(0, 56);
    display.print("WiFi:");
    display.print(WiFi.status() == WL_CONNECTED ? "OK" : "X");
    display.print(" MQTT:");
    display.print(client.connected() ? "OK" : "X");
  } else {
    display.setCursor(0, 0);
    display.println("Sensors (2/2)");
    printLine(16, "Lux",   f1(g_lx));
    printLine(28, "Mag",   String(g_mag));
    printLine(40, "Dist",  i1(g_dist));
    display.setCursor(0, 56);
    display.print("IP:");
    display.print(WiFi.localIP());
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    ok_oled = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Booting...");
    display.display();
  }
  wifiConnect();
  mqttConnect();
  initSensors();
  drawOLED();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!client.connected()) mqttConnect();
  client.loop();

  unsigned long now = millis();

  if (now - lastPageSwitch >= pagePeriod) {
    curPage = (curPage == 0) ? 1 : 0;
    lastPageSwitch = now;
    drawOLED();
  }

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

  g_t = t; g_h = h; g_p = p; g_lx = lx; g_mag = mag; g_dist = dist;

  String js = "{";
  js += "\"t\":" + (isnan(t) ? String("null") : String(t,1));
  js += ",\"hum\":" + (isnan(h) ? String("null") : String(h,1));
  js += ",\"pressure\":" + (isnan(p) ? String("null") : String(p,1));
  js += ",\"lux\":" + (isnan(lx) ? String("null") : String(lx,1));
  js += ",\"mag\":" + String(mag);
  js += ",\"dist\":" + (dist < 0 ? String("null") : String(dist));
  js += "}";
 client.publish(topicPub, js.c_str());
  Serial.println(js);

  drawOLED();
}

