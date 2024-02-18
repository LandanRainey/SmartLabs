/*********
 Landan Rainey
 This file will send data from the ESP32 to the Raspberry Pi
 The data will be sent as string values as follows:
 "Date,Time,Temperature,Humidity,Pressure,Air_Quality,Person_Count"

 As of now, the data will publish once every minute.
*********/
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "stdlib.h"

/*#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10*/

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);


// Network SSID/Password combination
const char* ssid = "Juddy Budde's";
const char* password = "MuBeta2023";

// MQTT Broker IP address;
const char * mqtt_server = "192.168.1.159";

// Data packet FORMAT IS: "11/03/2023,20:04,65.23,099,78,098,1" 
// ["Date,Time,Temperature(Celsius),Humidity%,Pressure(kilaPascals),Air_Quality(Resistance Value),OccupancyCount]

String packet;
String date="02/09/2024";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
const char* device = "ESP32";
//const char* user = "pi";
//const char* pass = "SmartLabs";

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("BME680 async test"));
  // Default settings
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    client.connect("ESP32");
  }
}

void loop() {
  //setup();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) { // Publishes new data every 5 seconds
    lastMsg = now;
    
      // Tell BME680 to begin measurement.
    unsigned long endTime = bme.beginReading();
    if (endTime == 0) {
      Serial.println(F("Failed to begin reading :("));
      return;
    }
    Serial.print(F("Reading started at "));
    Serial.print(millis());
    Serial.print(F(" and will finish at "));
    Serial.println(endTime);

    Serial.println(F("You can do other work during BME680 measurement."));
    delay(50); // This represents parallel work.
    // There's no need to delay() until millis() >= endTime: bme.endReading()
    // takes care of that. It's okay for parallel work to take longer than
    // BME680's measurement time.

    // Obtain measurement results from BME680. Note that this operation isn't
    // instantaneous even if milli() >= endTime due to I2C/SPI latency.
    if (!bme.endReading()) {
      Serial.println(F("Failed to complete reading :("));
      return;
    }
    Serial.print(F("Reading completed at "));
    Serial.println(millis());
    

    // Concatenate all data into a packet
    packet = date + "," + "21:15" + "," + bme.temperature + "," + (bme.pressure/100.0) + "," + bme.humidity + "," + (bme.gas_resistance/1000.0) + "," + bme.readAltitude(SEALEVELPRESSURE_HPA);

    Serial.print(F("Temperature = "));
    Serial.print(bme.temperature);
    Serial.println(F(" *C"));

    Serial.print(F("Pressure = "));
    Serial.print(bme.pressure / 100.0);
    Serial.println(F(" hPa"));

    Serial.print(F("Humidity = "));
    Serial.print(bme.humidity);
    Serial.println(F(" %"));

    Serial.print(F("Gas = "));
    Serial.print(bme.gas_resistance / 1000.0);
    Serial.println(F(" KOhms"));

    Serial.print(F("Approx. Altitude = "));
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(F(" m"));

    Serial.println();
    delay(2000);


    client.publish("python/mqtt", packet.c_str());
  }
}
