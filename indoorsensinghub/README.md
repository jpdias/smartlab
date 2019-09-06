# Docs

> Indoor Sensing Hub

## Run

``` $ pio run ```

## Deploy to board

``` $ platformio run --target upload ```

## JSON Schema

```

[
  {
    "name": "Text display",
    "href": "/things/textDisplay",
    "@context": "https://iot.mozilla.org/schemas",
    "@type": [
      "TextDisplay"
    ],
    "properties": {
      "text": {
        "type": "string",
        "href": "/things/textDisplay/properties/text"
      }
    }
  },
  {
    "name": "Temperature & Humidity Sensor",
    "href": "/things/dht11",
    "@context": "https://iot.mozilla.org/schemas",
    "@type": [],
    "properties": {
      "temperatureC": {
        "type": "number",
        "href": "/things/dht11/properties/temperatureC"
      },
      "temperatureF": {
        "type": "number",
        "href": "/things/dht11/properties/temperatureF"
      },
      "humidity": {
        "type": "number",
        "href": "/things/dht11/properties/humidity"
      },
      "heatIndex": {
        "type": "number",
        "href": "/things/dht11/properties/heatIndex"
      },
      "dewPoint": {
        "type": "number",
        "href": "/things/dht11/properties/dewPoint"
      }
    }
  },
  {
    "name": "PIR Motion sensor",
    "href": "/things/motion",
    "@context": "https://iot.mozilla.org/schemas",
    "@type": [],
    "properties": {
      "motion": {
        "type": "boolean",
        "href": "/things/motion/properties/motion"
      }
    }
  }
]

```