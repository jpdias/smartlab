#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Thing.h>
#include <WebThingAdapter.h>
#include <DHT.h>
#include <TaskScheduler.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ArduinoJson.h>
#include "PubSubClient.h"
#include "arduino_secrets.h"
#include "utils.h"


#define PIN_STATE_HIGH HIGH
#define PIN_STATE_LOW LOW

char *ssid = WLAN_SSID;
char *password = WLAN_PASS;

#define SCL_PIN 5 //D1
#define SDA_PIN 4 //D2
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(-1); // -1 = no reset pin

#define PIR 13       //D7
#define DHTPIN 2     //D4
#define STATUSLED 14 //D5

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define ID "SENSOR-NODE-3-MOZIoT"
const char* mqttServer = "192.168.0.134";
const int mqttPort = 1883;

String lastText = "moz://a iot";

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  lastText = "";
  for (unsigned int i = 0; i < length; i++) {
    lastText += (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println("<");
}


WiFiClient espClient;
PubSubClient mqtt_client(mqttServer, mqttPort, espClient);

void mqttReconnect(){
    while (!mqtt_client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (mqtt_client.connect(ID)) {
            Serial.println("connected");
            mqtt_client.subscribe("moz/displaytext");
        } else {
            Serial.print("Error: failed with state ");
            Serial.println(mqtt_client.state());
            delay(2000);
        }
    }
    
}


//Adafruit_SSD1306  display(0x3c, SDA_PIN, SCL_PIN);
const int textHeight = 8;
const int textWidth = 6;
const int width = 128;
const int height = 32;

int state = false; // by default, no motion detected

WebThingAdapter *adapter;

const char *dht11Types[] = {nullptr};
ThingDevice indoor("dht11", "Temperature & Humidity Sensor", dht11Types);
ThingProperty indoorTempC("temperatureC", "", NUMBER, nullptr);
ThingProperty indoorTempF("temperatureF", "", NUMBER, nullptr);
ThingProperty indoorHum("humidity", "", NUMBER, nullptr);
ThingProperty indoorHeatIndex("heatIndex", "", NUMBER, nullptr);
ThingProperty indoorDewPoint("dewPoint", "", NUMBER, nullptr);

const char *PIRTypes[] = {nullptr};
ThingDevice motion("motion", "PIR Motion sensor", PIRTypes);
ThingProperty motionDetected("motion", "", BOOLEAN, nullptr);

const char *textDisplayTypes[] = {"TextDisplay", nullptr};
ThingDevice textDisplay("textDisplay", "Text display", textDisplayTypes);
ThingProperty text("text", "", STRING, nullptr);

void displayString(const String &str)
{
  //Serial.println("f-displayString");
  int len = str.length();
  int strWidth = len * textWidth;
  int strHeight = textHeight;
  int scale = width / strWidth;
  if (strHeight > strWidth / 2)
  {
    scale = height / strHeight;
  }
  int x = width / 4 - strWidth * scale / 2;
  int y = height / 4 - strHeight * scale / 2;

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(scale);
  display.setCursor(x, y);
  display.println(str);
  display.flip();
  display.display();
}

void readMotionData()
{
  //Serial.println("f-readMotionData");

  ThingPropertyValue value;
  value.boolean = state;
  motionDetected.setValue(value);
}

void readDHT11data()
{
  Serial.println("Updating DHT data.");
  //Serial.println("f-readDHT11data");

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  // Compute heat index
  float hi = dht.computeHeatIndex(t, h);
  // Compute dew point -- NOAA
  float dp = dewPoint(t, h);

  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  ThingPropertyValue value;

  value.number = t;
  indoorTempC.setValue(value);
  value.number = f;
  indoorTempF.setValue(value);
  value.number = h;
  indoorHum.setValue(value);
  value.number = hi;
  indoorHeatIndex.setValue(value);
  value.number = dp;
  indoorDewPoint.setValue(value);

  
  const int capacity_dht=JSON_OBJECT_SIZE(6);
  StaticJsonBuffer<capacity_dht>doc_mem_dht;
  JsonObject& doc_dht = doc_mem_dht.createObject();
  doc_dht["node-id"] = ID;
  doc_dht["sensor"] = "dht11";
  doc_dht["hum-percent"] = h;
  doc_dht["temp-C"] = t;
  doc_dht["heatindex-C"] = hi;
  doc_dht["dewpoint"] = dp;
  char output_dht[256];
  doc_dht.printTo(output_dht);
  Serial.println(output_dht);
  mqtt_client.publish("temp-hum-readings", output_dht);
}

ICACHE_RAM_ATTR void motionDetectedInterrupt()
{
  Serial.println("Motion Detected!");
  state = !state;
  
  const int capacity_motion = JSON_OBJECT_SIZE(3);
  StaticJsonBuffer<capacity_motion>doc_mem_motion;
  JsonObject& doc_motion = doc_mem_motion.createObject();
  doc_motion["node-id"] = ID;
  doc_motion["sensor"] = "pir-motion";
  doc_motion["motion-bool"] = state;
  char output_motion[128];
  doc_motion.printTo(output_motion);
  Serial.println(output_motion);
  mqtt_client.publish("motion-readings", output_motion);
}

Task t1(60000, TASK_FOREVER, &readDHT11data);
Scheduler runner;

void setup()
{
  //Initialize serial:
  Serial.begin(9600);

  runner.init();
  Serial.println("Initialized scheduler");

  runner.addTask(t1);
  Serial.println("added t1");

  WiFi.begin(ssid, password);

  // no dhcp

  pinMode(STATUSLED, OUTPUT);
  pinMode(PIR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIR), motionDetectedInterrupt, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  delay(2000);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    digitalWrite(STATUSLED, PIN_STATE_HIGH);
    delay(500);
    digitalWrite(STATUSLED, PIN_STATE_LOW);
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mqttReconnect();
  mqtt_client.setCallback(callback);

  digitalWrite(STATUSLED, PIN_STATE_HIGH);

  dht.begin();

  adapter = new WebThingAdapter("indoorsensor", WiFi.localIP());

  ThingPropertyValue value;
  value.string = &lastText;
  text.setValue(value);

  textDisplay.addProperty(&text);
  adapter->addDevice(&textDisplay);

  indoor.addProperty(&indoorTempC);
  indoor.addProperty(&indoorTempF);
  indoor.addProperty(&indoorHum);
  indoor.addProperty(&indoorHeatIndex);
  indoor.addProperty(&indoorDewPoint);
  adapter->addDevice(&indoor);

  motion.addProperty(&motionDetected);
  adapter->addDevice(&motion);

  adapter->begin();

  readDHT11data();
  readMotionData();
  displayString(lastText);

  t1.enable();
  Serial.println("Enabled task t1");
}

void loop()
{
  mqttReconnect();
  mqtt_client.loop();
  runner.execute();
  readMotionData();
  displayString(lastText);
  adapter->update();
}