#include <Homie.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// Defines
#define DHTPIN 5 
#define DHTTYPE DHT11
#define sleepTimeInSeconds 300
#define measurementsPerInterval 5

// Settings
ADC_MODE(ADC_VCC);

// Gloabal Variables
DHT dht(DHTPIN, DHTTYPE);
volatile float v = 0;
volatile float t = 0;
volatile float h = 0;
volatile float raw_voltage = 0;
volatile float raw_temperatur = 0;
volatile float raw_humidity = 0;

volatile float voltageOffset = 0.272;

// Nodes
HomieNode MeasurementNode("Measurements", "Measurements", "string");

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:
      // Read five times all values
      for(int i = 0; i < measurementsPerInterval; i++) {
        raw_voltage += ESP.getVcc();
        raw_temperatur += dht.readTemperature();
        raw_humidity += dht.readHumidity();
      }
      // Calculate the mean of all values
      v = (raw_voltage/measurementsPerInterval/1000) + voltageOffset;
      t = raw_temperatur/measurementsPerInterval;
      h = raw_humidity/measurementsPerInterval;

      Homie.getLogger() << "Send Volate: " << v << endl;
      MeasurementNode.setProperty("Voltage").send(String(v));
      Homie.getLogger() << "Send Temperatur: " << t << endl;
      MeasurementNode.setProperty("Temperatur").send(String(t));
      Homie.getLogger() << "Send Humidity: " << h << endl;
      MeasurementNode.setProperty("Humidity").send(String(h));
      Homie.getLogger() << "✔ MQTT is ready, prepare to sleep..." << endl;
      digitalWrite(D2, LOW);
      Homie.prepareToSleep();
      break;
    case HomieEventType::READY_TO_SLEEP:
      Homie.getLogger() << "Ready to sleep" << endl;
      ESP.deepSleep(sleepTimeInSeconds*1e6);
      break;
    default:
      break;
  }
}

void setup() {
// put your setup code here, to run once:
    //Homie.disableLedFeedback();
    Serial.begin(115200);
    Serial << endl << endl;

    dht.begin();
    pinMode(D2, OUTPUT);
    
    Homie_setFirmware("TempSensor", "0.0.1");
    Homie.onEvent(onHomieEvent);

    MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
    MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");
    MeasurementNode.advertise("Voltage").setName("Volt").setRetained(true).setDatatype("float");

    digitalWrite(D2, HIGH);

    Homie.setup();
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}