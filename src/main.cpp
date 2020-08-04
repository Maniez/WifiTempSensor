#include <Homie.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Ticker.h>

// Defines
#define DHTPIN 5 
#define DHTTYPE DHT11
#define MeasurementTimeInSeconds 300
#define measurementsPerInterval 5

// Settings
ADC_MODE(ADC_VCC);
Ticker measurmentTimer;

// Gloabal Variables
DHT dht(DHTPIN, DHTTYPE);
volatile float v = 0;
volatile float t = 0;
volatile float h = 0;
volatile float raw_voltage = 0;
volatile float raw_temperatur = 0;
volatile float raw_humidity = 0;

volatile float voltageOffset = -0.13;
volatile float deltaTemp = 0.3;

bool globalEnableMeasurment = false;

// RTC Memory
struct {
  uint32_t bootCount = 0;
  float temperatur = 0;
} rtcMemory;


// Nodes
HomieNode MeasurementNode("Measurements", "Measurements", "string");

void doMeasurement(void) {
  // Read five times all values
  v = 0;
  t = 0;
  h = 0;
  for(int i = 0; i < measurementsPerInterval; i++) {
    v += ESP.getVcc();
    t += dht.readTemperature();
    h += dht.readHumidity();
    delay(100);
  }
  // Calculate the mean of all values
  v = (v/measurementsPerInterval/1000) + voltageOffset;
  t = t/measurementsPerInterval;
  h = h/measurementsPerInterval;
  Serial << "Send Volate: " << v << endl;
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

    Serial << "Decide wheather return or send data" << endl;
    if(abs((rtcMemory.temperatur - t)) < deltaTemp) {
      Serial << "Temperatur difference is to small, return" << endl;
      globalEnableMeasurment = false;
      return;
    } 

    MeasurementNode.setProperty("Voltage").send(String(v));
    MeasurementNode.setProperty("Temperatur").send(String(t));
    MeasurementNode.setProperty("Humidity").send(String(h));

    rtcMemory.temperatur = t;

    globalEnableMeasurment = false;
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;

  // Start Homie  
  Homie_setFirmware("TempSensor", "0.1.0");
  Homie.setLoopFunction(mainHomieLoop);
  measurmentTimer.attach(MeasurementTimeInSeconds, enableMeasurment);
  dht.begin();

  MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Voltage").setName("Volt").setRetained(true).setDatatype("float");

  Homie.setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  Homie.loop();
}