#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>  
#include "ArduinoJson.h"
#include "PubSubClient.h"

#include "config.h"


#define BUZZER D1
#define LED_RED D2
#define LED_GREEN D3
#define LED_BLUE D6
#define BUTTON D5

const char* mqttServer = SERVER_IP;
const int mqttPort = MQTT_PORT;

WiFiClient espClient;
void callbackFx(char* topic, byte* payload, unsigned int length);
PubSubClient mqtt_client(mqttServer, mqttPort, callbackFx, espClient);

void setled(int color) { //red(1)//green(2)//blue(3)//off(0)
    switch (color)
    {
    case 1:
        digitalWrite(LED_GREEN,LOW);
        digitalWrite(LED_BLUE,LOW);
        digitalWrite(LED_RED,HIGH);
        break;
    case 2:
        digitalWrite(LED_BLUE,LOW);
        digitalWrite(LED_RED,LOW);
        digitalWrite(LED_GREEN,HIGH);
        break;
    case 3:
        digitalWrite(LED_RED,LOW);
        digitalWrite(LED_GREEN,LOW);
        digitalWrite(LED_BLUE,HIGH);
        break;
    case 0:
        digitalWrite(LED_RED,LOW);
        digitalWrite(LED_GREEN,LOW);
        digitalWrite(LED_BLUE,LOW);
        break;
    default:
        digitalWrite(LED_RED,LOW);
        digitalWrite(LED_GREEN,LOW);
        digitalWrite(LED_BLUE,LOW);
        break;
    }
    
}

void stopBuzzer(){
    noTone(BUZZER);
}

void alarm(int duration, int type) {
    switch (type)
    {
    case 0:
        for(int x = 0; x < duration; x++){ 
            tone(BUZZER, 330, 250);
            delay(250);
            tone(BUZZER, 311, 250);
            delay(250);
        }
        break;
    case 1:
        for(int j = 10; j <= duration; j+=10){
            for(int i = 200; i <= 800; i++)  //Increase frequency from 200HZ to 800HZ in a circular manner.
            {
                tone(BUZZER,i);
                delay(5);     
            }
            delay(1000);
            for(int i = 800; i >= 200; i--) //Reduce frequency from 800HZ to 200HZ in a circular manner.
            {
                tone(BUZZER,i);
                delay(10);
            }
            noTone(BUZZER);
        }
        break;
    default:
        for (size_t i = 0; i < duration; i++){
            tone(BUZZER, 1500, 1000);
            delay(1000);
            noTone(BUZZER);
        }
        break;
    }            
}

void callbackFx(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived @ ");
  Serial.println(topic);
  payload[length] = '\0';
  String strTopic = String((char*)topic);
  String msg = String((char*)payload);
  
  if (strTopic == "alarm/led/red") {
    setled(1);
  } 
  else if (strTopic == "alarm/led/green") {
    setled(2);
  }
  else if (strTopic == "alarm/led/blue") {
    setled(3);
  } 
  else if (strTopic == "alarm/led/off") {
    setled(0);
  }
  else if (strTopic == "alarm/buzzer/on") {
    StaticJsonDocument<200> jsonBuffer; 
    DeserializationError error = deserializeJson(jsonBuffer, msg);
    if(msg == ""){
        alarm(2,2);
    } else if (!error) {
        int type = jsonBuffer["type"];
        int duration = jsonBuffer["duration"];
        alarm(duration,type);
    } else {
        mqtt_client.publish("alarm/debug", "Error parsing duration");
    }
  } 
  else if (strTopic == "alarm/buzzer/off") { 
    stopBuzzer();
  }
  
}

ICACHE_RAM_ATTR void buttonPressed(){

    const int cap_button = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<cap_button>doc_mem_button;
    doc_mem_button["node-id"] = ID;
    doc_mem_button["actuator"] = "button";
    doc_mem_button["value"] = "clicked";
    char output_btn[128];
    serializeJson(doc_mem_button, output_btn);
    Serial.println(output_btn);

    mqtt_client.publish("alarm/button", output_btn);
}

void mqttReconnect(){
     while (!mqtt_client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (mqtt_client.connect(ID)) {
            Serial.println("connected");
            mqtt_client.subscribe("alarm/#");
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
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(BUTTON, INPUT_PULLUP);

    
    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonPressed, FALLING);

    WiFi.begin(WLAN_SSID, WLAN_PASS);

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        Serial.print(".");
        digitalWrite(LED_BUILTIN, HIGH);
    }
    Serial.println();

    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    if (!MDNS.begin(ID)) {
        Serial.println("Error setting up MDNS responder!");
    }

    Serial.println("mDNS responder started");
    MDNS.addService("mqtt", "tcp", 1883);
    MDNS.addServiceTxt("mqtt", "button", "alarm/button");
    MDNS.addServiceTxt("mqtt", "led", "alarm/led|/red|/green|/blue");
    MDNS.addServiceTxt("mqtt", "buzzer", "alarm/buzzer|/on|/off, {'type': 0|1,'duration': int}");
    mqttReconnect();

    digitalWrite(LED_BUILTIN, HIGH);
}



// the loop function runs over and over again forever
void loop() {
    mqttReconnect();
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    mqtt_client.loop();
    MDNS.update();
}