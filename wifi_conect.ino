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

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Підключення до WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi підключено!");

  dht.begin();

  if (!aht.begin()) Serial.println("⚠️ AHT20 не знайдено!");
  else Serial.println("✅ AHT20 підключено!");

  if (!bmp.begin(0x77)) Serial.println("⚠️ BMP280 не знайдено!");
  else Serial.println("✅ BMP280 підключено!");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ Втрата WiFi — перепідключення...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    return;
  }

  if (millis() - prevSendTime >= sendInterval) {
    prevSendTime = millis();

    float dhtTemp = dht.readTemperature();
    float dhtHum = dht.readHumidity();

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    float bmpTemp = bmp.readTemperature();
    float bmpPress = bmp.readPressure() / 100.0F;

    int soilValue = analogRead(SOIL_PIN);
    float soilHum = map(soilValue, 1023, 200, 0, 100);
    soilHum = constrain(soilHum, 0, 100);

    String json = "{";
    json += "\"dhtTemp\":" + String(dhtTemp, 1) + ",";
    json += "\"dhtHum\":" + String(dhtHum, 1) + ",";
    json += "\"ahtTemp\":" + String(temp.temperature, 1) + ",";
    json += "\"ahtHum\":" + String(humidity.relative_humidity, 1) + ",";
    json += "\"bmpTemp\":" + String(bmpTemp, 1) + ",";
    json += "\"bmpPress\":" + String(bmpPress, 1) + ",";
    json += "\"soilHum\":" + String(soilHum, 1);
    json += "}";

    Serial.println("🌱 Надсилаю дані...");
    Serial.println(json);

    client.beginRequest();
    client.post(SERVER_PATH);
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
}