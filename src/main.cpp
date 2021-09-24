#include <Homie.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// Defines
#define sleepTimeInSeconds 20

// Settings

// Gloabal Variables

// RTC Memory
struct RTC{
  uint32_t bootCount;
  float batteryLevel;
  bool rainSensor;
} RTC_Memory;


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
      ESP.rtcUserMemoryWrite(0, (uint32_t *) &RTC_Memory, sizeof(RTC_Memory));
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
  Serial.begin(115200);
  Serial << endl << endl;

  // Print Wake-Up infos
  Serial << "Boot reason: " << ESP.getResetReason() << endl;
  if (ESP.getResetReason() == "Power On") {
    RTC_Memory.bootCount = 0;
    RTC_Memory.batteryLevel = 0;
    RTC_Memory.rainSensor = false;
    ESP.rtcUserMemoryWrite(0, (uint32_t *) &RTC_Memory, sizeof(RTC_Memory));
  }

  ESP.rtcUserMemoryRead(0, (uint32_t *) &RTC_Memory, sizeof(RTC_Memory));
  Serial << String(RTC_Memory.bootCount) << endl << endl;
  RTC_Memory.bootCount++;

  // Startup finished 
  // Start pre productiv Code
  // Do measurement of Temp,humidty and voltage

  Serial << "Read Batterylevel and Calc wether send is need" << endl;  
  float batteryLevel_temp = analogRead(A0) * 420 / 420 / 1000; 
  Serial << "Batterylevel new: " << batteryLevel_temp << " <-> old: " << RTC_Memory.batteryLevel << endl;

  if(abs(batteryLevel_temp - RTC_Memory.batteryLevel) < 0.01) {
    Serial << "No need for send" << endl;
    ESP.rtcUserMemoryWrite(0, (uint32_t *) &RTC_Memory, sizeof(RTC_Memory));
    ESP.deepSleep(sleepTimeInSeconds*1e6);
  } else {
    RTC_Memory.batteryLevel = batteryLevel_temp;

    WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    
    // Start Homie  
    //Homie.disableLedFeedback();
    Homie_setFirmware("TempSensor", "0.1.0");
    //Homie.setLoopFunction(mainHomieLoop);
    Homie.onEvent(onHomieEvent);

    /*
    MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
    MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");
    MeasurementNode.advertise("Voltage").setName("Volt").setRetained(true).setDatatype("float");
    MeasurementNode.advertise("Bootcount").setName("Bootcount").setRetained(true).setDatatype("float");
    */

    Homie.setup();
  }
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}