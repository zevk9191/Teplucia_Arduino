#include <WiFiS3.h>
#include <ArduinoWebsockets.h>
#include <ArduinoHttpClient.h>
#include "pass.h"

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

#define DHTPIN 2
#define DHTTYPE DHT11
#define SOIL_PIN A0

DHT dht(DHTPIN, DHTTYPE);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

using namespace websockets;

WebsocketsClient wsClient;
WiFiClient wifi;  // це для 443 WiFiSSLClient а для 80 WiFiClient wifi;
HttpClient client(wifi, SERVER_HOST, SERVER_PORT);

unsigned long prevSendTime = 0;
const unsigned long sendInterval = 600000;  // 10 хвилин

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.status() == WL_CONNECTED) Serial.println("WiFi підключено!");
}

void setup() {
  Serial.begin(9600);
  connectWiFi();

  wsClient.onMessage(onMessage);
  wsClient.connect(SERVER_WS);  // наприклад ws://your-server-ip:3000
  wsClient.send("HELLO_FROM_ARDUINO");

  Serial.println("✅ WebSocket підключено");
}

void loop() {
  connectWiFi();
  wsClient.poll();  // підтримує зв’язок активним

  if (millis() - prevSendTime >= sendInterval) {
    prevSendTime = millis();
    sendSensorData();
  }
}

void onMessage(WebsocketsMessage msg) {
  if (msg.data() == "REFRESH") {
    sendSensorData();
  }
}

// ----------- функція надсилання даних ----------
void sendSensorData() {
  float dhtTemp = dht.readTemperature();

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  float bmpPress = bmp.readPressure() / 100.0F;

  int soilValue = analogRead(SOIL_PIN);
  float soilHum = map(soilValue, 1023, 200, 0, 100);
  soilHum = constrain(soilHum, 0, 100);

  String json = "{";
  json += "\"DHT11\":{\"temp\":" + String(dhtTemp, 1) + "},";
  json += "\"AHT20\":{\"hum\":" + String(humidity.relative_humidity, 1) + "},";
  json += "\"BMP280\":{\"press\":" + String(bmpPress, 1) + "},";
  json += "\"SOIL\":{\"hum\":" + String(soilHum, 1) + "}";
  json += "}";

  Serial.println("Надсилаю дані.");
  Serial.println(json);

  client.beginRequest();
  client.post(SERVER_PATH);
  client.sendHeader("x-device-key", DEVICE_API_KEY);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", json.length());
  client.beginBody();
  client.print(json);
  client.endRequest();

  String response = "";
  while (client.available()) {
    response += client.read();
  }
  Serial.print("Код відповіді: ");
  Serial.println(client.responseStatusCode());
  Serial.print("Відповідь: ");
  Serial.println(response);
  Serial.println("------");

  client.stop();
}