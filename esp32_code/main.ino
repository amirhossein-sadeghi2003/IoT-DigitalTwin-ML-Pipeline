#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define LED_PIN 5
#define LED_COUNT 6
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "your_ssid";
const char* pass = "your_password";

const char* mqttHost = "broker.hivemq.com";
const uint16_t mqttPort = 1883;
const char* topicPub = "iot/test/sensors";
const char* topicSubAct = "iot/cmd/act";
const char* topicSubMode = "iot/cmd/mode";
const char* topicStatusMode = "iot/status/mode";

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

float g_t = NAN, g_h = NAN, g_p = NAN, g_lx = NAN;
int g_mag = 0, g_dist = -1;

bool heater_on=0, cooler_on=0, flood_on=0, window_open=0, ai_mode=0, alarm_on=0;
bool alarm_blink=0;
unsigned long last_blink=0;
const unsigned long blink_period=300;

void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(ssid, pass);
  int c = 0;
  while (WiFi.status() != WL_CONNECTED && c < 40) {
    delay(500);
    c++;
  }
}

void led_update() {
  strip.clear();
  if (heater_on) strip.setPixelColor(0, strip.Color(255,0,0));
  if (cooler_on) strip.setPixelColor(1, strip.Color(0,0,255));
  if (flood_on)  strip.setPixelColor(2, strip.Color(180,180,180));
  if (window_open) strip.setPixelColor(3, strip.Color(0,180,0));
  if (ai_mode) strip.setPixelColor(4, strip.Color(0,180,0));
  else strip.setPixelColor(4, strip.Color(0,0,180));
  if (alarm_on) { if (alarm_blink) strip.setPixelColor(5, strip.Color(255,0,0)); }
  strip.show();
}

void mqttCallback(char* topic, byte* msg, unsigned int len) {
  String m = "";
  for (unsigned int i=0;i<len;i++) m += (char)msg[i];

  if (String(topic) == topicSubAct) {
    if (m.startsWith("heater=")) heater_on = (m.endsWith("1"));
    else if (m.startsWith("cooler=")) cooler_on = (m.endsWith("1"));
    else if (m.startsWith("flood=")) flood_on = (m.endsWith("1"));
    else if (m.startsWith("window=")) window_open = (m.endsWith("1"));
    else if (m.startsWith("alarm=")) alarm_on = (m.endsWith("1"));
    led_update();
  } else if (String(topic) == topicSubMode) {
    if (m=="ai") ai_mode=1;
    else ai_mode=0;
    led_update();
    String js = "{\"mode\":\"" + String(ai_mode ? "ai" : "rules") + "\"}";
    client.publish(topicStatusMode, js.c_str());
  }
}

void mqttConnect() {
  client.setServer(mqttHost, mqttPort);
  client.setCallback(mqttCallback);
  while (!client.connected()) {
    String cid = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (client.connect(cid.c_str())) {
      client.subscribe(topicSubAct);
      client.subscribe(topicSubMode);
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

String f1(float x) { if (isnan(x)) return String("--"); return String(x,1); }
String i1(int x) { if (x<0) return String("--"); return String(x); }

void drawOLED() {
  if (!ok_oled) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Sensors");
  printLine(16, "T(C)", f1(g_t));
  printLine(28, "Hum(%)", f1(g_h));
  printLine(40, "Lux", f1(g_lx));
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
    display.setCursor(0,0);
    display.println("Booting...");
    display.display();
  }
  strip.begin();
  strip.setBrightness(40);
  strip.show();
  wifiConnect();
  mqttConnect();
  initSensors();
  led_update();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!client.connected()) mqttConnect();
  client.loop();

  unsigned long now = millis();
  if (alarm_on) {
    if (now - last_blink >= blink_period) {
      last_blink = now;
      alarm_blink = !alarm_blink;
      led_update();
    }
  } else {
    if (alarm_blink) { alarm_blink=0; led_update(); }
  }

  if (now - lastSend < sendPeriod) return;
  lastSend = now;

  float t=NAN,h=NAN,p=NAN,lx=NAN;
  int mag=0,dist=-1;

  if (ok_bme) {
    t=bme.readTemperature();
    h=bme.readHumidity();
    p=bme.readPressure()/100.0f;
  }
  if (ok_bh) lx=bh.readLightLevel();
  if (ok_lox) {
    VL53L0X_RangingMeasurementData_t m;
    lox.rangingTest(&m, false);
    if (m.RangeStatus==0) dist=m.RangeMilliMeter;
  }
  mag=(digitalRead(PIN_MAG)==LOW)?1:0;

  g_t=t; g_h=h; g_p=p; g_lx=lx; g_mag=mag; g_dist=dist;

  String js="{";
  js+="\"t\":"+(isnan(t)?String("null"):String(t,1));
  js+=",\"hum\":"+(isnan(h)?String("null"):String(h,1));
  js+=",\"lux\":"+(isnan(lx)?String("null"):String(lx,1));
  js+=",\"mag\":"+String(mag);
  js+=",\"dist\":"+(dist<0?String("null"):String(dist));
  js+="}";

  client.publish(topicPub, js.c_str());
  drawOLED();
}

