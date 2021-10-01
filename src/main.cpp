#include <Arduino.h>
#include <Homie.h>
#include <SPI.h>
#include <Wire.h>


// Defines
#define sleepTimeInSeconds 30
#define oversamplingSamples 16
#define batterySendCount 45

// Settings

// Gloabal Variables

// RTC Memory
struct RTC {
    uint32_t bootCount;
    float batteryLevel;
    bool rainSensor;
} RTC_Memory;

// Nodes
HomieNode RainSensorNode("Battery", "Battery", "level");

void onHomieEvent(const HomieEvent &event) {
    switch (event.type) {
        case HomieEventType::MQTT_READY:
            RainSensorNode.setProperty("BatteryLevel").send(String(RTC_Memory.batteryLevel));
            Homie.getLogger() << "Send battery level" << endl;
            RainSensorNode.setProperty("BootCount").send(String(RTC_Memory.bootCount));
            Homie.getLogger() << "Send boot count" << endl;
            Homie.getLogger() << "✔ MQTT is ready, prepare to sleep..." << endl;
            Homie.prepareToSleep();
            break;
        case HomieEventType::READY_TO_SLEEP:
            Homie.getLogger() << "Ready to sleep" << endl;
            ESP.rtcUserMemoryWrite(0, (uint32_t *)&RTC_Memory, sizeof(RTC_Memory));
            ESP.deepSleep(sleepTimeInSeconds * 1e6);
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
    Serial << endl;

    //GPIO init
    pinMode(D2, INPUT);

    // Print Wake-Up infos
    Serial << "Boot reason: " << ESP.getResetReason() << endl;
    if (ESP.getResetReason() == "Power On") {
        RTC_Memory.bootCount = 0;
        RTC_Memory.batteryLevel = 0;
        RTC_Memory.rainSensor = false;
        ESP.rtcUserMemoryWrite(0, (uint32_t *)&RTC_Memory, sizeof(RTC_Memory));
    }

    ESP.rtcUserMemoryRead(0, (uint32_t *)&RTC_Memory, sizeof(RTC_Memory));
    Serial << String(RTC_Memory.bootCount) << endl;
    RTC_Memory.bootCount++;

    bool startHomie = false;

    // Startup finished
    // Start pre productiv Code
    // Do measurement of Temp,humidty and voltage
    if (RTC_Memory.bootCount % batterySendCount == 0) {
        Serial << "Read Batterylevel, do average and deside wether send is need or not" << endl;
        uint32_t adcReadout = 0;
        for (uint16_t averageCounter = 0; averageCounter < oversamplingSamples; averageCounter++) {
            adcReadout += analogRead(A0);
        }
        float batteryLevel_temp = ((float)adcReadout / oversamplingSamples) * 420 / 100 / 1000;

        Serial << "Batterylevel new: " << batteryLevel_temp << " <-> old: " << RTC_Memory.batteryLevel << endl;
        if (abs(batteryLevel_temp - RTC_Memory.batteryLevel) >= 0.01) {
            RTC_Memory.batteryLevel = batteryLevel_temp;
            startHomie = true;
        }
    }

    bool rainSensor_temp = (digitalRead(D2) == 1) ? true : false;
    Serial << "Sensor new: " << rainSensor_temp << " <-> old: " << RTC_Memory.rainSensor << endl;
    if (rainSensor_temp != RTC_Memory.rainSensor) {
        RTC_Memory.rainSensor = rainSensor_temp;
        startHomie = true;
    }

    if (startHomie) {
        WiFi.forceSleepWake();
        WiFi.mode(WIFI_STA);

        // Start Homie
        //Homie.disableLedFeedback();
        Homie_setFirmware("TempSensor", "0.1.0");
        //Homie.setLoopFunction(mainHomieLoop);
        Homie.onEvent(onHomieEvent);

        RainSensorNode.advertise("BatteryLevel").setName("Level").setDatatype("float");
        RainSensorNode.advertise("BootCount").setName("BootCnt").setDatatype("integer");
        RainSensorNode.advertise("RainState").setName("Rain").setDatatype("boolean");

        Homie.setup();
    } else {
        Serial << "No send triggerd, Bootcount: " << (RTC_Memory.bootCount % batterySendCount) << endl;
        ESP.rtcUserMemoryWrite(0, (uint32_t *)&RTC_Memory, sizeof(RTC_Memory));
        ESP.deepSleep(sleepTimeInSeconds * 1e6);
    }
}

void loop() {
    // put your main code here, to run repeatedly:
    Homie.loop();
}