#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "ArduinoJson.h"
#include "PubSubClient.h"
#include "arduino_secrets.h"


int pirPin = D6;
int trigPin = D3;
int echoPin = D4;
int doorPin = D5;

int count = 0;   //which y pin we are selecting

#define DHTTYPE DHT22
#define ID "SENSOR-NODE-2"
const char* mqttServer = "192.168.0.134";
const int mqttPort = 1883;

WiFiClient espClient;

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

volatile byte motion_detected = LOW;
volatile byte door_open = LOW;

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


ICACHE_RAM_ATTR void motion_detected_fx(){
    motion_detected = digitalRead(pirPin);
    const int capacity_motion = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<capacity_motion>doc_motion;
    doc_motion["node-id"] = ID;
    doc_motion["sensor"] = "pir-motion";
    doc_motion["motion-bool"] = motion_detected;
    char output_motion[128];
    serializeJson(doc_motion, output_motion);
    Serial.println(output_motion);
    mqtt_client.publish("motion-readings", output_motion);
}

ICACHE_RAM_ATTR void door_fx(){
    door_open = digitalRead(doorPin);
    const int capacity_door = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<capacity_door>doc_door;
    doc_door["node-id"] = ID;
    doc_door["sensor"] = "door-status";
    doc_door["door-bool"] = door_open;
    char output_door[128];
    serializeJson(doc_door, output_door);
    Serial.println(output_door);
    mqtt_client.publish("door-readings", output_door);

}

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT); // Sets the echoPin as an Input
    pinMode(pirPin, INPUT);
    pinMode(doorPin, INPUT_PULLUP);      

    attachInterrupt(digitalPinToInterrupt(pirPin), motion_detected_fx, CHANGE);
    attachInterrupt(digitalPinToInterrupt(doorPin), door_fx, CHANGE);

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

    
}

int last_max_distance = 0;
float duration, distance;

// the loop function runs over and over again forever
void loop() {

    mqttReconnect();

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = (duration*.0343)/2;
    Serial.print("Distance: ");
    Serial.println(distance);
    
    if((int) distance > 3000 || (int) distance < 1) return;

    if(abs((int) distance - (int) last_max_distance) > 20){ //changes above 20 cm
        last_max_distance = distance;
        digitalWrite(LED_BUILTIN, HIGH);
        const int capacity_entry = JSON_OBJECT_SIZE(4);
        StaticJsonDocument<capacity_entry>doc_entry;
        doc_entry["node-id"] = ID;
        doc_entry["sensor"] = "sonar-distance";
        doc_entry["entry-bool"] = HIGH;
        doc_entry["sonar-distance"] = distance;
        char output_entry[128];
        serializeJson(doc_entry, output_entry);
        Serial.println(output_entry);
        mqttReconnect();
        mqtt_client.publish("sonar-readings", output_entry);
    }

    
    delay(300);
}