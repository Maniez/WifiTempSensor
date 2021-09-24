#include <Homie.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// Defines
#define sleepTimeInSeconds 30

// Settings
ADC_MODE(ADC_VCC);

// Gloabal Variables

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

void setup() {
  // send wifi directly to sleep to reduce power consumption
  WiFi.forceSleepBegin();  
  yield();

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

  /*
  Serial << "Decide wheather goto sleep or send data" << endl;
  if(abs((rtcMemory.temperatur - t)) < deltaTemp) {
    Serial << "Temperatur difference is to small, send back to sleep" << endl;
    Serial << "Runtime in ms: " << millis() << endl;
    ESP.deepSleep(sleepTimeInSeconds*1e6);
  } 
  */

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
  MeasurementNode.advertise("Bootcount").setName("Bootcount").setRetained(true).setDatatype("float");

  Homie.setup();
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}