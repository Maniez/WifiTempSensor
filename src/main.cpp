#include <Homie.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// Defines
#define sleepTimeInSeconds 20

// Settings
ADC_MODE(ADC_VCC);

// Gloabal Variables

// RTC Memory
struct NVM{
  uint32_t bootCount;
  float batteryLevel;
  bool rainSensor;
  uint32_t snedCounter;
} NVM_Memory;


// Nodes
HomieNode MeasurementNode("Measurements", "Measurements", "string");

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:
      Homie.getLogger() << "✔ MQTT is ready, prepare to sleep..." << endl;
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
  // send wifi directly to sleep to reduce power consumption
  WiFi.forceSleepBegin();  
  yield();

  // Init LED and Serial
  //Homie.disableLedFeedback();
  Serial.begin(115200);
  Serial << endl << endl;

  // Print Wake-Up infos
  Serial << ESP.getResetReason() << endl;
  if (ESP.getResetReason() == "Power On") {
    NVM_Memory.bootCount = 0;
    NVM_Memory.batteryLevel = 0;
    NVM_Memory.rainSensor = false;
    NVM_Memory.snedCounter = 0;
    ESP.rtcUserMemoryWrite(0, (uint32_t *) &NVM_Memory, sizeof(NVM_Memory));
  }

  ESP.rtcUserMemoryRead(0, (uint32_t *) &NVM_Memory, sizeof(NVM_Memory));
  Serial << String(NVM_Memory.bootCount) << endl << endl;
  NVM_Memory.bootCount++;
  ESP.rtcUserMemoryWrite(0, (uint32_t *) &NVM_Memory, sizeof(NVM_Memory));

  // Startup finished 
  // Start pre productiv Code
  // Do measurement of Temp,humidty and voltage

  Serial << "Simulate 10s of processing...";
  delay(10000);
  Serial << "Decide wheather goto sleep or send data" << endl;
  if(NVM_Memory.bootCount % 5 != 0) {
    Serial << "Bootcount modulo" << (NVM_Memory.bootCount % 5) << endl;
    ESP.deepSleep(sleepTimeInSeconds*1e6);
  } 

  ESP.rtcUserMemoryWrite(0, (uint32_t *) &NVM_Memory, sizeof(NVM_Memory));
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
  // End pre productiv Code


  // Start productive code
  // Start Homie  
  Homie_setFirmware("TempSensor", "0.1.0");
  Homie.onEvent(onHomieEvent);

  /*
  MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Voltage").setName("Volt").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Bootcount").setName("Bootcount").setRetained(true).setDatatype("float");
  */

  Homie.setup();
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}