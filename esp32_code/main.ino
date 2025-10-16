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
#define LED_PIN 5
#define LED_COUNT 6
#define BUZZER_PIN 23

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* wifi_ssid = "02F1G_9979c0";
const char* wifi_pass = "wlan66863f";
const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;
const char* mqtt_topic_sensors = "iot/test/sensors";
const char* mqtt_topic_control = "iot/cmd/act";
const char* mqtt_topic_predictions = "iot/model/predictions"; 

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

Adafruit_BME280 temp_sensor;
BH1750 light_sensor;
Adafruit_VL53L0X distance_sensor;

bool is_temp_sensor_ok = false;
bool is_light_sensor_ok = false;
bool is_distance_sensor_ok = false;
bool is_display_ok = false;

const int magnet_pin = 18;
unsigned long last_data_send = 0;
const unsigned long data_send_interval = 3000;

uint8_t display_page = 0;
unsigned long last_page_change = 0;
const unsigned long page_change_interval = 5000;

float current_temp = NAN, current_humidity = NAN, current_pressure = NAN, current_light = NAN;
int current_magnet = 0, current_distance = -1;


bool is_ai_mode = false;
bool is_heater_on = false;
bool is_cooler_on = false;
bool is_flood_on = false;
bool is_window_open = false;


bool manual_is_heater_on = false;
bool manual_is_cooler_on = false;
bool manual_is_flood_on = false;
bool manual_is_window_open = false;

bool is_alarm_on = false;
unsigned long last_led_blink = 0;
const unsigned long led_blink_interval = 500;
bool led_state = false;
bool is_buzzer_on = false;
unsigned long buzzer_start_time = 0;
const unsigned long buzzer_duration = 3000;
const int buzzer_frequency = 2000;


void update_actuators() {
    
    strip.setPixelColor(0, is_heater_on ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0));
   
    strip.setPixelColor(1, is_cooler_on ? strip.Color(0, 0, 255) : strip.Color(0, 0, 0));
    
    strip.setPixelColor(2, is_flood_on ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0));
    
    strip.setPixelColor(3, is_window_open ? strip.Color(255, 255, 255) : strip.Color(0, 0, 0));
    
   
    strip.setPixelColor(5, is_ai_mode ? strip.Color(0, 0, 255) : strip.Color(0, 255, 0));

    strip.show();
}

void connect_wifi() {
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.begin(wifi_ssid, wifi_pass);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
        delay(500);
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
    } else {
        Serial.println("WiFi failed");
    }
}

void mqtt_message_handler(char* topic, byte* message, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)message[i];
    }
    Serial.println("Message on " + String(topic) + ": " + msg);

    
    bool command_received = false;
    bool new_mode = is_ai_mode;

    
    if (String(topic) == mqtt_topic_control) {
        if (msg == "heater=1") { 
            manual_is_heater_on = true; 
            if (!is_ai_mode) is_heater_on = true; 
            command_received = true; 
        } 
        else if (msg == "heater=0") { 
            manual_is_heater_on = false; 
            if (!is_ai_mode) is_heater_on = false; 
            command_received = true; 
        } 
        else if (msg == "cooler=1") { 
            manual_is_cooler_on = true; 
            if (!is_ai_mode) is_cooler_on = true; 
            command_received = true; 
        } 
        else if (msg == "cooler=0") { 
            manual_is_cooler_on = false; 
            if (!is_ai_mode) is_cooler_on = false; 
            command_received = true; 
        } 
        else if (msg == "flood=1") { 
            manual_is_flood_on = true; 
            if (!is_ai_mode) is_flood_on = true; 
            command_received = true; 
        } 
        else if (msg == "flood=0") { 
            manual_is_flood_on = false; 
            if (!is_ai_mode) is_flood_on = false; 
            command_received = true; 
        } 
        else if (msg == "window=1") { 
            manual_is_window_open = true; 
            if (!is_ai_mode) is_window_open = true; 
            command_received = true; 
        } 
        else if (msg == "window=0") { 
            manual_is_window_open = false; 
            if (!is_ai_mode) is_window_open = false; 
            command_received = true; 
        } 
        else if (msg == "alarm=1") { is_alarm_on = true; Serial.println("Alarm on"); } 
        else if (msg == "alarm=0") { 
            is_alarm_on = false; 
            strip.setPixelColor(4, strip.Color(0, 0, 0)); 
            if (is_buzzer_on) { noTone(BUZZER_PIN); is_buzzer_on = false; Serial.println("Buzzer off"); }
            Serial.println("Alarm off"); 
            command_received = true; 
        } 
        else if (msg == "mode=ai") { new_mode = true; Serial.println("AI mode"); } 
        else if (msg == "mode=rule") { new_mode = false; Serial.println("Rule mode"); }
    } 
    
    else if (String(topic) == mqtt_topic_predictions && is_ai_mode) {
        if (msg == "heater=1") { is_heater_on = true; command_received = true; } 
        else if (msg == "heater=0") { is_heater_on = false; command_received = true; } 
        else if (msg == "cooler=1") { is_cooler_on = true; command_received = true; } 
        else if (msg == "cooler=0") { is_cooler_on = false; command_received = true; } 
        else if (msg == "flood=1") { is_flood_on = true; command_received = true; } 
        else if (msg == "flood=0") { is_flood_on = false; command_received = true; } 
        else if (msg == "window=1") { is_window_open = true; command_received = true; } 
        else if (msg == "window=0") { is_window_open = false; command_received = true; }
    }

    
    if (new_mode != is_ai_mode) {
        is_ai_mode = new_mode;
        if (!is_ai_mode) { 
            is_heater_on = manual_is_heater_on;
            is_cooler_on = manual_is_cooler_on;
            is_flood_on = manual_is_flood_on;
            is_window_open = manual_is_window_open;
        }
        update_actuators(); 
    } else if (command_received) {
        if (is_ai_mode && String(topic) == mqtt_topic_control) {
             
        } else {
           
            update_actuators();
        }
    }
}

void connect_mqtt() {
    mqtt_client.setServer(mqtt_server, mqtt_port);
    mqtt_client.setCallback(mqtt_message_handler);
    while (!mqtt_client.connected()) {
        String client_id = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        if (mqtt_client.connect(client_id.c_str())) {
            mqtt_client.subscribe(mqtt_topic_control);
            mqtt_client.subscribe(mqtt_topic_predictions);
            mqtt_client.publish(mqtt_topic_sensors, "{\"status\":\"online\"}");
            Serial.println("MQTT connected");
        } else {
            delay(800);
            Serial.println("MQTT failed, retrying...");
        }
    }
}

void setup_sensors() {
    if (temp_sensor.begin(0x76) || temp_sensor.begin(0x77)) {
        is_temp_sensor_ok = true;
        Serial.println("BME280 OK");
    } else {
        Serial.println("BME280 failed");
    }
    if (light_sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        is_light_sensor_ok = true;
        Serial.println("BH1750 OK");
    } else {
        Serial.println("BH1750 failed");
    }
    if (distance_sensor.begin()) {
        is_distance_sensor_ok = true;
        Serial.println("VL53L0X OK");
    } else {
        Serial.println("VL53L0X failed");
    }
    pinMode(magnet_pin, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
}

void print_display_line(int16_t y, const char* label, const String &value) {
    display.setCursor(0, y);
    display.print(label);
    display.print(": ");
    display.print(value);
}

String format_float(float value) {
    if (isnan(value)) return String("--");
    return String(value, 1);
}

String format_int(int value) {
    if (value < 0) return String("--");
    return String(value);
}

void update_display() {
    if (!is_display_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (display_page == 0) {
        display.setCursor(0, 0);
        display.println("Sensors (1/2)");
        print_display_line(16, "Temp(C)", format_float(current_temp));
        print_display_line(28, "Humidity(%)", format_float(current_humidity));
        print_display_line(40, "Pressure(hPa)", format_float(current_pressure));
        display.setCursor(0, 56);
        display.print("WiFi:");
        display.print(WiFi.status() == WL_CONNECTED ? "OK" : "X");
        display.print(" MQTT:");
        display.print(mqtt_client.connected() ? "OK" : "X");
    } else {
        display.setCursor(0, 0);
        display.println("Sensors (2/2)");
        print_display_line(16, "Light", format_float(current_light));
        print_display_line(28, "Magnet", String(current_magnet));
        print_display_line(40, "Distance", format_int(current_distance));
        display.setCursor(0, 56);
        display.print("IP:");
        display.print(WiFi.localIP());
    }

    display.display();
}

void test_leds() {
    for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(255, 255, 255));
    }
    strip.show();
    delay(1000);
    strip.clear();
    strip.show();
    Serial.println("LED test done");
}

void test_buzzer() {
    tone(BUZZER_PIN, 2000);
    delay(1000);
    noTone(BUZZER_PIN);
    Serial.println("Buzzer test done");
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    strip.begin();
    strip.setBrightness(50);
    strip.show();
    test_leds();
    test_buzzer();
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        is_display_ok = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Booting...");
        display.display();
        Serial.println("Display OK");
    }
    connect_wifi();
    connect_mqtt();
    setup_sensors();
    update_display();
  
    update_actuators(); 
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) connect_wifi();
    if (!mqtt_client.connected()) connect_mqtt();
    mqtt_client.loop();

    unsigned long current_time = millis();

    if (current_time - last_page_change >= page_change_interval) {
        display_page = (display_page == 0) ? 1 : 0;
        last_page_change = current_time;
        update_display();
    }

    
    if (is_alarm_on) {
        if (current_time - last_led_blink >= led_blink_interval) {
            last_led_blink = current_time;
            led_state = !led_state;
            
            strip.setPixelColor(4, led_state ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0));
            strip.show();
        }
    }

    if (is_buzzer_on && (current_time - buzzer_start_time >= buzzer_duration)) {
        noTone(BUZZER_PIN);
        is_buzzer_on = false;
        Serial.println("Buzzer stopped");
    }

    if (current_time - last_data_send < data_send_interval) return;
    last_data_send = current_time;

    
    float temp = NAN, humidity = NAN, pressure = NAN, light = NAN;
    int magnet = 0;
    int distance = -1;

    if (is_temp_sensor_ok) {
        temp = temp_sensor.readTemperature();
        humidity = temp_sensor.readHumidity();
        pressure = temp_sensor.readPressure() / 100.0f;
    }
    if (is_light_sensor_ok) {
        light = light_sensor.readLightLevel();
    }
    if (is_distance_sensor_ok) {
        VL53L0X_RangingMeasurementData_t measurement;
        distance_sensor.rangingTest(&measurement, false);
        if (measurement.RangeStatus == 0) {
            distance = measurement.RangeMilliMeter;
        }
    }

    magnet = (digitalRead(magnet_pin) == LOW) ? 1 : 0;

    current_temp = temp;
    current_humidity = humidity;
    current_pressure = pressure;
    current_light = light;
    current_magnet = magnet;
    current_distance = distance;

    
    if (is_alarm_on && distance >= 0 && !is_buzzer_on) {
        Serial.println("Buzzer triggered: Alarm on, distance = " + String(distance));
        is_buzzer_on = true;
        buzzer_start_time = current_time;
        tone(BUZZER_PIN, buzzer_frequency);
    }

   
    String json_data = "{";
    json_data += "\"temp\":" + (isnan(temp) ? String("null") : String(temp, 1));
    json_data += ",\"humidity\":" + (isnan(humidity) ? String("null") : String(humidity, 1));
    json_data += ",\"pressure\":" + (isnan(pressure) ? String("null") : String(pressure, 1));
    json_data += ",\"light\":" + (isnan(light) ? String("null") : String(light, 1));
    json_data += ",\"magnet\":" + String(magnet);
    json_data += ",\"distance\":" + (distance < 0 ? String("null") : String(distance));
    json_data += ",\"alarm\":" + String(is_alarm_on ? 1 : 0);
    json_data += "}";
    mqtt_client.publish(mqtt_topic_sensors, json_data.c_str());
    Serial.println(json_data);

    update_display();
}
