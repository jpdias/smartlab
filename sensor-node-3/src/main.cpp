#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>  
#include <ESP8266WebServer.h>
#include "ArduinoJson.h"
#include "DHT.h"
#include "PubSubClient.h"
#include <TaskScheduler.h>
#include <SPI.h>
#include <RH_NRF24.h>

#include "config.h"

const char* mqttServer = SERVER_IP;
const int mqttPort = MQTT_PORT;

WiFiClient espClient;

ESP8266WebServer server(80);   // Create a webserver object that listens for HTTP request on port 80

void handleRoot();
void handleDht();              // function prototypes for HTTP handlers
void handleNotFound();

RH_NRF24 nrf24(2, 4); 

#define DHTPIN D1
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
PubSubClient mqtt_client(mqttServer, mqttPort, espClient);

char output_dht[128];

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
    yield();
}

void readDHT(){
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    float hic = dht.computeHeatIndex(t, h, false);
    const int capacity_dht=JSON_OBJECT_SIZE(5);
    StaticJsonDocument<capacity_dht>doc_dht;
    doc_dht["node-id"] = ID;
    doc_dht["sensor"] = "dht22";
    doc_dht["hum-percent"] = h;
    doc_dht["temp-C"] = t;
    doc_dht["heatindex-C"] = hic;
    serializeJson(doc_dht, output_dht);
    Serial.println(output_dht);
    mqtt_client.publish("temp-hum-readings", output_dht);

    // Now wait for a reply
    String msg = String(String(ID)+",");
    msg += String(t);
    msg += ",";
    msg += String(h);
    msg += '\0';
    
    Serial.println(msg.c_str());
    nrf24.send((uint8_t*)msg.c_str(), msg.length());
    nrf24.waitPacketSent();
    yield();
    delay(400);
}

Task readSensorsTask(60000, TASK_FOREVER, &readDHT);
Scheduler runner;


void setup() {
    Serial.begin(115200);
    dht.begin();

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

    if (!MDNS.begin("sensor-node-4")) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }
    MDNS.addService("mqtt", "tcp", 1883);
    MDNS.addServiceTxt("mqtt", "temp-hum", "temp-hum-readings");
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "temp-hum", "temp-hum-readings");
    Serial.println("mDNS responder started");

    mqttReconnect();

    runner.init();
    Serial.println("Initialized scheduler");
    
    runner.addTask(readSensorsTask);
    readSensorsTask.enable();
    Serial.println("added readSensorsTask");


    //rf433
    if (!nrf24.init())
        Serial.println("init failed");
    // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
    if (!nrf24.setChannel(3))
        Serial.println("setChannel failed");
    if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
        Serial.println("setRF failed");    

    yield();
    //http server
    server.on("/", handleRoot);  
    server.on("/dht", handleDht);               // Call the 'handleRoot' function when a client requests URI "/"
    server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

    server.begin();                           // Actually start the server
    Serial.println("HTTP server started");
}

void loop() {
    // Wait a few seconds between measurements.
    mqttReconnect();
    MDNS.update();
    mqtt_client.loop();
    server.handleClient();
    runner.execute();
    yield();
}

void handleRoot() {
    server.send(200, "text/plain", "/dht");   // Send HTTP status 200 (Ok) and send some text to the browser/client
    yield();
}

void handleDht() {
    server.send(200, "application/json", output_dht);   // Send HTTP status 200 (Ok) and send some text to the browser/client
    yield();
}

void handleNotFound(){
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
    yield();
}