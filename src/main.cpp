#include <Homie.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

ADC_MODE(ADC_VCC);

#define DHTPIN 5 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

HomieNode dummyNode("Dummy", "Dummy", "switch");
float raw_voltage = 0;

unsigned long previousMillis = 0;

// Updates DHT readings every 10 seconds
const long interval = 10000; 

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:
      raw_voltage = ESP.getVcc();
      dummyNode.setProperty("Temp").send(String(raw_voltage));
      delay(500);
      Homie.getLogger() << "✔ MQTT is ready, prepare to sleep..." << endl;
      Homie.prepareToSleep();
      delay(100);
      break;
    case HomieEventType::READY_TO_SLEEP:
      Homie.getLogger() << "Ready to sleep" << endl;
      ESP.deepSleep(60e6, WAKE_RFCAL);
      break;
    default:
      break;
  }
}

void loopHandler() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
      dummyNode.setProperty("Temp").send(String(t));
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
      dummyNode.setProperty("Humidity").send(String(h));
    }
  }
}

void setup() {
// put your setup code here, to run once:
    Serial.begin(115200);
    Serial << endl << endl;

    dht.begin();

    Homie_setFirmware("TempSensor", "0.0.1");
    //Homie.onEvent(onHomieEvent);

    dummyNode.advertise("control").setName("test").setRetained(true).setDatatype("boolean");

    Homie.setLoopFunction(loopHandler);

    Homie.setup();
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}