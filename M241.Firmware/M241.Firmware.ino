#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Arduino_JSON.h>
#include <WiFiS3.h>
#include <MQTTClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "secrets.h"
#include "config.h"

const char* wiFiSsid = WIFI_SSID;
const char* wiFiPass = WIFI_PASS;
const char* mqttUser = MQTT_USER;
const char* mqttPass = MQTT_PASS;
const char* mqttHost = MQTT_HOST;
const int mqttPort = MQTT_PORT;
const char* mqttQueue = MQTT_QUEUE;
const boolean debuggingEnabled = DEBUGGING_ENABLED;
const float TEMP_OFFSET = -2.0;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

// Adafruit_BME680 bme(&Wire); // I2C
//Adafruit_BME680 bme(&Wire1); // example of I2C on another bus
Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

String macAddress;
WiFiClient wiFiClient;
MQTTClient mqttClient(1024);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

void setup() {
  Serial.begin(9600);
  if (debuggingEnabled) {
    while (!Serial);
  }

  initSensor();

  macAddress = getMacAddress();
  connectWiFi();

  timeClient.begin();
  timeClient.update();

  mqttClient.begin(mqttHost, mqttPort, wiFiClient);
  connectMqtt();
}

void loop() {
  timeClient.update();
  mqttClient.loop();

  if (bme.performReading()) {
    if (WiFi.status() == WL_CONNECTED) {
      if (mqttClient.connected()) {
        publishSensorData();
      } else {
        reconnectMqtt();
      }
    } else {
      reconnectWiFi();
    }
  } else {
    Serial.println("Failed to perform BME680 sensor reading :(");
  }

  delay(1000);
}

void initSensor() {
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
}

String getMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void connectWiFi() {
  int wiFiStatus = WL_IDLE_STATUS;
  while (wiFiStatus != WL_CONNECTED) {
    if (wiFiStatus != WL_IDLE_STATUS) {
      delay(5000);
    }
    wiFiStatus = attemptWiFiConnection();
  }

  Serial.print("Connected to network named: ");
  Serial.println(wiFiSsid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

int attemptWiFiConnection() {
  Serial.print("Attempting to connect to Network named: ");
  Serial.println(wiFiSsid);
  return WiFi.begin(wiFiSsid, wiFiPass);
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!attemptMqttConnection()) {
        delay(5000);
      }
    } else {
      reconnectWiFi();
    }
  }
  Serial.println("MQTT connection established!");
}

bool attemptMqttConnection() {
  Serial.print("Attempting to connect to MQTT ");
  Serial.print(mqttHost);
  Serial.print(":");
  Serial.println(mqttPort);
  return mqttClient.connect(("arduinoClient-" + macAddress).c_str(), mqttUser, mqttPass);
}

void reconnectWiFi() {
  Serial.println("Disconnected from WiFi, trying to reconnect...");
  connectWiFi();
}

void reconnectMqtt() {
  Serial.println("Disconnected from MQTT server, trying to reconnect...");
  connectMqtt();
}

void publishSensorData() {
  JSONVar json;
  json["macAddress"] = macAddress;
  json["timestamp"] = timeClient.getEpochTime();
  json["temperature"] = bme.temperature + TEMP_OFFSET;
  json["humidity"] = bme.humidity;
  json["pressure"] = bme.pressure;
  json["gas"] = bme.gas_resistance;

  String payload = JSON.stringify(json);
  if (mqttClient.publish(mqttQueue, payload.c_str())) {
    Serial.println("Published data (QoS = exactly once)");
  } else {
    Serial.println("Failed to publish");
    Serial.println("Payload was: " + payload);
  }
}
