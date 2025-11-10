#include <WiFiS3.h>
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

WiFiClient wifi;  // це для 443 WiFiSSLClient а для 80 WiFiClient wifi;
HttpClient client(wifi, SERVER_HOST, SERVER_PORT);

unsigned long prevSendTime = 0;
const unsigned long sendInterval = 10000;  // 10 секунд

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.status() == WL_CONNECTED) Serial.println("WiFi підключено!");
}

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

  Serial.println("Надсилаю дані...");
  Serial.println(json);

  client.beginRequest();
  client.post(SERVER_PATH);
  client.sendHeader("x-device-key", DEVICE_API_KEY);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", json.length());
  client.beginBody();
  client.print(json);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }

  Serial.print("Код відповіді: ");
  Serial.println(statusCode);
  Serial.print("Відповідь: ");
  Serial.println(response);
  Serial.println("------");

  client.stop();
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  aht.begin();
  bmp.begin(0x77);
  connectWiFi();
}

void loop() {
  if (millis() - prevSendTime >= sendInterval) {
    prevSendTime = millis();
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    sendSensorData();
  }
}