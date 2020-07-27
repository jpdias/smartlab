#include <Arduino.h>
#include <SPI.h>
#include <RH_NRF24.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>  
#include "ArduinoJson.h"
#include "PubSubClient.h"
#include "config.h"

// Singleton instance of the radio driver
RH_NRF24 nrf24(2, 4);


const char* mqttServer = SERVER_IP;
const int mqttPort = MQTT_PORT;

WiFiClient espClient;
PubSubClient mqtt_client(mqttServer, mqttPort, espClient);

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


void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  if (!nrf24.init())
      Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(3))
      Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
      Serial.println("setRF failed");

  Serial.println("nrf24 Connected");

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      Serial.print(".");
      digitalWrite(LED_BUILTIN, HIGH);
  }

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(ID)) {
      Serial.println("Error setting up MDNS responder!");
  }

  Serial.println("mDNS responder started");
  MDNS.addService("mqtt", "tcp", 1883);
  MDNS.addServiceTxt("mqtt", "broker", ID);
  mqttReconnect();
}

void loop()
{
  if (nrf24.available()) {
    // Should be a message for us now
    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf24.recv(buf, &len)) {
      Serial.println((char *)buf);
      digitalWrite(LED_BUILTIN, HIGH);
      mqtt_client.publish(ID,(char *)buf);
    }
    else {
      Serial.println('{"error": "recv failed"}');
      mqtt_client.publish(ID,(const char*)'{"error": "recv failed"}');
    }
  } else {
    mqttReconnect();
    mqtt_client.loop();
    MDNS.update();
  }
}
