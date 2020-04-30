#include <Homie.h>

ADC_MODE(ADC_VCC);

HomieNode dummyNode("Dummy", "Dummy", "switch");
float raw_voltage = 0;

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

void setup() {
// put your setup code here, to run once:
    Serial.begin(115200);
    Serial << endl << endl;

    Homie_setFirmware("TempSensor", "0.0.1");
    Homie.onEvent(onHomieEvent);

    dummyNode.advertise("control").setName("test").setRetained(true).setDatatype("boolean");

    Homie.setup();
}

void loop() {
// put your main code here, to run repeatedly:
    Homie.loop();
}