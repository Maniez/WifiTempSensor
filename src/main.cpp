#include <Homie.h>
#include <DHT.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <SPI.h>
#include <Ticker.h>

// Defines
#define DHTPIN 5 
#define DHTTYPE DHT11
#define MeasurementTimeInSeconds 300
#define measurementsPerInterval 5

// Gloabal Variables
DHT dht(DHTPIN, DHTTYPE);
Ticker measurmentTimer;

volatile float t = 0;
volatile float h = 0;

bool globalEnableMeasurment = true;

// Nodes
HomieNode MeasurementNode("Measurements", "Measurements", "string");

void doMeasurement(void) {
  // Read five times all values
  t = 0;
  h = 0;
  for(int i = 0; i < measurementsPerInterval; i++) {
    t += dht.readTemperature();
    h += dht.readHumidity();
    delay(100);
  }
  // Calculate the mean of all values
  t = t/measurementsPerInterval;
  h = h/measurementsPerInterval;
  Serial << "Send Temperatur: " << t << endl;
  Serial << "Send Humidity: " << h << endl;
}

void enableMeasurment(void){
  globalEnableMeasurment = true;
}

void mainHomieLoop(void) {
  if(globalEnableMeasurment == true) {
    // Do measurement of Temp,humidty and voltage
    Serial << "Do Measurement" << endl;
    doMeasurement();

    MeasurementNode.setProperty("Temperatur").send(String(t));
    MeasurementNode.setProperty("Humidity").send(String(h));

    globalEnableMeasurment = false;
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;

  // Start Homie  
  Homie_setFirmware("TempSensor", "0.2.0");
  Homie.setLoopFunction(mainHomieLoop);
  measurmentTimer.attach(MeasurementTimeInSeconds, enableMeasurment);
  dht.begin();

  MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");

  Homie.setup();
  ArduinoOTA.onStart([]() {
  Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin(); 
}

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  Homie.loop();
}