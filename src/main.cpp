#include <Homie.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// Defines
#define DHTPIN 5 
#define DHTTYPE DHT11
#define sleepTimeInSeconds 840
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

volatile float voltageOffset = -0.13;
volatile float deltaTemp = 0.5;

// RTC Memory
struct {
  uint32_t bootCount = 0;
  float temperatur = 0;
} rtcMemory;


// Nodes
HomieNode MeasurementNode("Measurements", "Measurements", "string");

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:
      
      MeasurementNode.setProperty("Voltage").send(String(v));
      MeasurementNode.setProperty("Temperatur").send(String(t));
      MeasurementNode.setProperty("Humidity").send(String(h));
      Homie.getLogger() << "✔ MQTT is ready, prepare to sleep..." << endl;
      Homie.prepareToSleep();
      break;
    case HomieEventType::READY_TO_SLEEP:
      Homie.getLogger() << "Ready to sleep" << endl;
      Serial << "Runtime in ms: " << millis() << endl;
      ESP.deepSleep(sleepTimeInSeconds*1e6);
      break;
    default:
      break;
  }
}

void doMeasurement(void) {
  //pinMode(D2, OUTPUT);
  //digitalWrite(D2, HIGH);
  dht.begin();
  delay(100);
  
  // Read five times all values
  for(int i = 0; i < measurementsPerInterval; i++) {
    raw_voltage += ESP.getVcc();
    raw_temperatur += dht.readTemperature();
    raw_humidity += dht.readHumidity();
    delay(100);
  }
  // Calculate the mean of all values
  v = (raw_voltage/measurementsPerInterval/1000) + voltageOffset;
  t = raw_temperatur/measurementsPerInterval;
  h = raw_humidity/measurementsPerInterval;
  Serial << "Send Volate: " << v << endl;
  Serial << "Send Temperatur: " << t << endl;
  Serial << "Send Humidity: " << h << endl;

  //digitalWrite(D2, LOW);
}

void setup() {
  // send wifi directly to sleep to reduce power consumption
  WiFi.forceSleepBegin();  
  yield();

  /*
  uint32_t CpuSpeed = ESP.getCpuFreqMHz();
  Serial << "CPU Speed:";
  Serial << String(CpuSpeed);
*/

  // Init LED and Serial
  Homie.disableLedFeedback();
  Serial.begin(115200);
  Serial << endl << endl;

  // Print Wake-Up infos
  Serial << ESP.getResetReason() << endl;
  if (ESP.getResetReason() == "Power On") {
    rtcMemory.bootCount = 0;
    rtcMemory.temperatur = 0;
    ESP.rtcUserMemoryWrite(0, (uint32_t *) &rtcMemory, sizeof(rtcMemory));
  }

  ESP.rtcUserMemoryRead(0, (uint32_t *) &rtcMemory, sizeof(rtcMemory));
  Serial << String(rtcMemory.bootCount) << endl << endl;
  rtcMemory.bootCount++;
  ESP.rtcUserMemoryWrite(0, (uint32_t *) &rtcMemory, sizeof(rtcMemory));

  // Startup finished 
  // Start pre productiv Code
  // Do measurement of Temp,humidty and voltage
  Serial << "Do Measurement" << endl;
  doMeasurement();

  Serial << "Decide wheather goto sleep or send data" << endl;
  if(abs((rtcMemory.temperatur - t)) < deltaTemp) {
    Serial << "Temperatur difference is to small, send back to sleep" << endl;
    Serial << "Runtime in ms: " << millis() << endl;
    ESP.deepSleep(sleepTimeInSeconds*1e6);
  } 

  rtcMemory.temperatur = t;
  ESP.rtcUserMemoryWrite(0, (uint32_t *) &rtcMemory, sizeof(rtcMemory));
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
  // End pre productiv Code


  // Start productive code
  // Start Homie  
  Homie_setFirmware("TempSensor", "0.1.0");
  Homie.onEvent(onHomieEvent);

  MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Voltage").setName("Volt").setRetained(true).setDatatype("float");

  Homie.setup();
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}