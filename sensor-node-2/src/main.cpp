#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "ArduinoJson.h"
#include "PubSubClient.h"
#include "arduino_secrets.h"

#define DHTTYPE DHT22
#define ID "sensor-node-2"
#define PIRPIN D6
#define DOORPIN D5

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
    motion_detected = digitalRead(PIRPIN);
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
    door_open = digitalRead(DOORPIN);
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
    pinMode(PIRPIN, INPUT);
    pinMode(DOORPIN, INPUT_PULLUP);      

    attachInterrupt(digitalPinToInterrupt(PIRPIN), motion_detected_fx, CHANGE);
    attachInterrupt(digitalPinToInterrupt(DOORPIN), door_fx, CHANGE);

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
    
    if (!MDNS.begin("sensor-node-2")) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }

    MDNS.addService("mqtt", "tcp", 1883);
    MDNS.addServiceTxt("mqtt", "motion", "motion-readings");
    MDNS.addServiceTxt("mqtt", "door", "door_readings");

    Serial.println("mDNS responder started");
    
}

// the loop function runs over and over again forever
void loop() {
    mqttReconnect();
    mqtt_client.loop();
    MDNS.update();
}