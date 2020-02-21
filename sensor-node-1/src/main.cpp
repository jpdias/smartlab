#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>  
#include "ArduinoJson.h"
#include "DHT.h"
#include "TroykaMQ.h"
#include "PubSubClient.h"
#include <TaskScheduler.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "config.h"

#define analog_read A0
#define temp_hum_sensor_dht22 D4
#define noise_sensor D5

#define s0 D1 //s0
#define s1 D2 //s1
#define s2 D3 //s2

int count = 0;   //which y pin we are selecting

#define DHTTYPE DHT22

const char* mqttServer = SERVER_IP;
const int mqttPort = MQTT_PORT;

WiFiClient espClient;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

PubSubClient mqtt_client(mqttServer, mqttPort, callback, espClient);

DHT dht(temp_hum_sensor_dht22, DHTTYPE);
MQ135 mq135(analog_read);
MQ9 mq9(analog_read);

volatile byte noise_detected = LOW;

ICACHE_RAM_ATTR void noise_detected_fx(){
    noise_detected = HIGH;
}

void readSensorsCallback(){

    digitalWrite(LED_BUILTIN, HIGH);

    //Gas Sensor
    digitalWrite(s0, LOW);//000
    digitalWrite(s1, LOW);
    digitalWrite(s2, LOW);

    mq135.calibrate();
    delay(100);
    
    float readRatio = mq135.readRatio();
    float co2val = mq135.readCO2();
    const int capacity_gas = JSON_OBJECT_SIZE(5);
    StaticJsonDocument<capacity_gas>doc_gas;
    doc_gas["node-id"] = ID;
    doc_gas["sensor"] = "MQ135";
    doc_gas["read-ratio"] = readRatio;
    doc_gas["co2-ppm"]= co2val;
    doc_gas["timestamp"] = timeClient.getEpochTime();
    char output_gas[128];
    serializeJson(doc_gas, output_gas);
    Serial.println(output_gas);
    mqtt_client.publish("gas-mq135-readings", output_gas);

    delay(100);

    //Light Sensor
    digitalWrite(s0, HIGH);//001
    digitalWrite(s1, LOW);
    digitalWrite(s2, LOW);
                
    int LDR_value = analogRead(analog_read); 
    double Vout=LDR_value*0.0048828125;
    int lux=500/(10*((5-Vout)/Vout));//use this equation if the LDR is in the upper part of the divider
    //int lux=(2500/Vout-500)/10;
    const int capacity_ldr=JSON_OBJECT_SIZE(5);
    StaticJsonDocument<capacity_ldr>doc_ldr;
    doc_ldr["node-id"] = ID;
    doc_ldr["sensor"] = "LDR";
    doc_ldr["analog-read"] = LDR_value;
    doc_ldr["lux"]= lux;
    doc_ldr["timestamp"] = timeClient.getEpochTime();
    char output_ldr[128];
    serializeJson(doc_ldr, output_ldr);
    Serial.println(output_ldr);
    mqtt_client.publish("lux-readings", output_ldr);

    delay(100);

    //MQ9 Sensor 
    digitalWrite(s0, LOW);
    digitalWrite(s1, HIGH); //010
    digitalWrite(s2, LOW);
    
    mq9.calibrate();
    delay(100);

    float readRatio_1 = mq9.readRatio();
    float co = mq9.readCarbonMonoxide();
    float ch4 = mq9.readMethane();
    float lpg = mq9.readLPG();

    const int capacity_gas_1 = JSON_OBJECT_SIZE(7);
    StaticJsonDocument<capacity_gas_1>doc_gas_1;
    doc_gas_1["node-id"] = ID;
    doc_gas_1["sensor"] = "MQ9";
    doc_gas_1["read-ratio"] = readRatio_1;
    doc_gas_1["co-ppm"]= co;
    doc_gas_1["ch4-ppm"]= ch4;
    doc_gas_1["lpg-ppm"]= lpg;
    doc_gas_1["timestamp"] = timeClient.getEpochTime();
    char output_gas_1[128];
    serializeJson(doc_gas_1, output_gas_1);
    Serial.println(output_gas_1);
    mqtt_client.publish("gas-mq9-readings", output_gas_1);

    delay(100);


    //Sound Sensor           

    const int capacity_noise = JSON_OBJECT_SIZE(4);
    StaticJsonDocument<capacity_noise>doc_noise;
    doc_noise["node-id"] = ID;
    doc_noise["sensor"] = "noise_sensor";
    doc_noise["noise-bool"] = noise_detected;
    doc_noise["timestamp"] = timeClient.getEpochTime();
    noise_detected = LOW;
    char output_noise[128];
    serializeJson(doc_noise, output_noise);
    Serial.println(output_noise);
    mqtt_client.publish("noise-readings", output_noise);

    delay(100);


    float h = dht.readHumidity();
    // Read temperature as Celsius (the default) (isFahrenheit = false)
    float t = dht.readTemperature(false);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
    const int capacity_dht=JSON_OBJECT_SIZE(6);
    StaticJsonDocument<capacity_dht>doc_dht;
    doc_dht["node-id"] = ID;
    doc_dht["sensor"] = "dht22";
    doc_dht["hum-percent"] = h;
    doc_dht["temp-C"] = t;
    doc_dht["heatindex-C"] = hic;
    doc_dht["timestamp"] = timeClient.getEpochTime();
    char output_dht[128];
    serializeJson(doc_dht, output_dht);
    Serial.println(output_dht);
    mqtt_client.publish("temp-hum-readings", output_dht);

    digitalWrite(LED_BUILTIN, LOW);
}

Task readSensorsTask(60000, TASK_FOREVER, &readSensorsCallback);
Scheduler runner;


void mqttReconnect(){
     while (!mqtt_client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (mqtt_client.connect(ID)) {
            Serial.println("connected");
        } else {
            Serial.print("Error: failed with state ");
            Serial.println(mqtt_client.state());
            delay(2000);
        }
    }
}

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(noise_sensor, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(noise_sensor), noise_detected_fx, RISING);

    dht.begin();

    pinMode(s0, OUTPUT);
    pinMode(s1, OUTPUT);
    pinMode(s2, OUTPUT);

    //MQ135 calibrate
    digitalWrite(s0, LOW);//000
    digitalWrite(s1, LOW);
    digitalWrite(s2, LOW);
    mq135.calibrate();
    Serial.print("Ro = ");
    Serial.println(mq135.getRo());

    //MQ9 Sensor 
    digitalWrite(s0, LOW);
    digitalWrite(s1, HIGH); //010
    digitalWrite(s2, LOW);            
    mq9.calibrate();
    Serial.print("Ro = ");
    Serial.println(mq9.getRo());

    WiFi.begin(WLAN_SSID, WLAN_PASS);

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    if (!MDNS.begin("sensor-node-1")) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }
    MDNS.addService("mqtt", "tcp", 1883);
    MDNS.addServiceTxt("mqtt", "co2", "gas-mq135-readings");
    MDNS.addServiceTxt("mqtt", "lux", "lux-readings");
    MDNS.addServiceTxt("mqtt", "co-ch4-lpg", "gas-mq9-readings");
    MDNS.addServiceTxt("mqtt", "temp-hum", "temp-hum-readings");
    MDNS.addServiceTxt("mqtt", "noise-detected", "noise-readings");
    Serial.println("mDNS responder started");
    
    mqttReconnect();

    runner.init();
    Serial.println("Initialized scheduler");
    
    runner.addTask(readSensorsTask);
    readSensorsTask.enable();
    Serial.println("added readSensorsTask");

    digitalWrite(LED_BUILTIN, LOW);

    timeClient.begin();
    delay(500);
}



// the loop function runs over and over again forever
void loop() {
    timeClient.update();
    delay(100);
    mqttReconnect();
    delay(100);
    MDNS.update();
    delay(100);
    mqtt_client.loop();
    delay(100);
    runner.execute();
}