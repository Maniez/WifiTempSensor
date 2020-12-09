#include <Homie.h>
#include <DHT.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <SPI.h>
#include <Ticker.h>
#include <RemoteDebug.h>  //https://github.com/JoaoLopesF/RemoteDebug

// Defines
#define DHTPIN 5 
#define DHTTYPE DHT11
#define MeasurementTimeInSeconds 300
#define measurementsPerInterval 5

#define DEBUG_EN

#ifdef DEBUG_EN
#define HOST_NAME "tempsensor-3"
#endif

// Gloabal Variables
DHT dht(DHTPIN, DHTTYPE);
Ticker measurmentTimer;

volatile float t = 0;
volatile float h = 0;

bool globalEnableMeasurment = true;

#ifdef DEBUG_EN
RemoteDebug Debug;

uint32_t mqtt_lost_counter = 0;
uint32_t mqtt_lost_reason[10] = {0};
uint32_t mqtt_lost_time[10] = {0};
uint32_t wifi_lost_counter = 0;
uint32_t wifi_lost_reason[10] = {0};
uint32_t wifi_lost_time[10] = {0};
#endif

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
    // Do measurement of Temp and humidty
    Serial << "Do Measurement" << endl;
    doMeasurement();

    MeasurementNode.setProperty("Temperatur").send(String(t));
    MeasurementNode.setProperty("Humidity").send(String(h));

    globalEnableMeasurment = false;
  }
}

#ifdef DEBUG_EN

// Process commands from RemoteDebug

void processCmdRemoteDebug() {
  
	String lastCmd = Debug.getLastCommand();

	if (lastCmd == "debug1") {

		// Print Debug variables
    debugD("Wifi disconnected %d times", wifi_lost_counter);
    for(uint32_t i = 0; (i < wifi_lost_counter) && (i < 10); i++) {
      debugD("Wifi disconnected reason for disconnect %d was: %d at time %d", i+1, wifi_lost_reason[i], wifi_lost_time[i]);
    }

    debugD("MQTT disconnected %d times", mqtt_lost_counter);
    for(uint32_t i = 0; (i < mqtt_lost_counter) && (i < 10); i++) {
      debugD("MQTT disconnected reason for disconnect %d was: %d at time %d", i+1, mqtt_lost_reason[i], mqtt_lost_time[i]);
    }


	} else if (lastCmd == "debug2") {

		// Clear Debug variables
    mqtt_lost_counter = 0;
    for(uint8_t i = 0; i<10; i++){
      mqtt_lost_reason[i] = 0;
    }

    wifi_lost_counter = 0;
    for(uint8_t i = 0; i<10; i++){
      wifi_lost_reason[i] = 0;
    }
  }
}
#endif

void onHomieEvent(const HomieEvent& event) {
  String helpCmd;
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE:
      // Do whatever you want when standalone mode is started
      break;
    case HomieEventType::CONFIGURATION_MODE:
      // Do whatever you want when configuration mode is started
      break;
    case HomieEventType::NORMAL_MODE:
      // Do whatever you want when normal mode is started
      break;
    case HomieEventType::OTA_STARTED:
      // Do whatever you want when OTA is started
      break;
    case HomieEventType::OTA_PROGRESS:
      // Do whatever you want when OTA is in progress

      // You can use event.sizeDone and event.sizeTotal
      break;
    case HomieEventType::OTA_FAILED:
      // Do whatever you want when OTA is failed
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      // Do whatever you want when OTA is successful
      break;
    case HomieEventType::ABOUT_TO_RESET:
      // Do whatever you want when the device is about to reset
      break;
    case HomieEventType::WIFI_CONNECTED:
      // Do whatever you want when Wi-Fi is connected in normal mode
#ifdef DEBUG_EN
      Debug.begin(HOST_NAME);
      Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
	    Debug.showColors(true); // Colors

      helpCmd = "debug1 - Debug 1\n";
      helpCmd.concat("debug2 - Debug 2");

      Debug.setHelpProjectsCmds(helpCmd);
      Debug.setCallBackProjectCmds(&processCmdRemoteDebug);
#endif
      // You can use event.ip, event.gateway, event.mask
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      // Do whatever you want when Wi-Fi is disconnected in normal mode
#ifdef DEBUG_EN
      debugD("Wifi disconnected reason %d at time %d", (int)event.wifiReason, (int)millis());
      wifi_lost_reason[wifi_lost_counter] = event.wifiReason == 0 ? 26 : event.wifiReason;
      wifi_lost_time[wifi_lost_counter] = millis();
      wifi_lost_counter++;
#endif
      // You can use event.wifiReason
      /*
        Wi-Fi Reason (souce: https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClientEvents/WiFiClientEvents.ino)
        0  SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
        1  SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
        2  SYSTEM_EVENT_STA_START                < ESP32 station start
        3  SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
        4  SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
        5  SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from AP
        6  SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
        7  SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from connected AP
        8  SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
        9  SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
        10 SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
        11 SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
        12 SYSTEM_EVENT_STA_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
        13 SYSTEM_EVENT_AP_START                 < ESP32 soft-AP start
        14 SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
        15 SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32 soft-AP
        16 SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
        17 SYSTEM_EVENT_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
        18 SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
        19 SYSTEM_EVENT_GOT_IP6                  < ESP32 station or ap or ethernet interface v6IP addr is preferred
        20 SYSTEM_EVENT_ETH_START                < ESP32 ethernet start
        21 SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
        22 SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
        23 SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
        24 SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
        25 SYSTEM_EVENT_MAX
      */
      break;
    case HomieEventType::MQTT_READY:
      // Do whatever you want when MQTT is connected in normal mode
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      // Do whatever you want when MQTT is disconnected in normal mode
#ifdef DEBUG_EN
      mqtt_lost_reason[mqtt_lost_counter] = (uint32_t)event.mqttReason == 0 ? 8 : (uint32_t)event.mqttReason;
      mqtt_lost_time[mqtt_lost_counter] = millis();
      mqtt_lost_counter++;
#endif
      // You can use event.mqttReason
      /*
        MQTT Reason (source: https://github.com/marvinroger/async-mqtt-client/blob/master/src/AsyncMqttClient/DisconnectReasons.hpp)
        0 TCP_DISCONNECTED
        1 MQTT_UNACCEPTABLE_PROTOCOL_VERSION
        2 MQTT_IDENTIFIER_REJECTED
        3 MQTT_SERVER_UNAVAILABLE
        4 MQTT_MALFORMED_CREDENTIALS
        5 MQTT_NOT_AUTHORIZED
        6 ESP8266_NOT_ENOUGH_SPACE
        7 TLS_BAD_FINGERPRINT
      */
      break;
    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED:
      // Do whatever you want when an MQTT packet with QoS > 0 is acknowledged by the broker

      // You can use event.packetId
      break;
    case HomieEventType::READY_TO_SLEEP:
      // After you've called `prepareToSleep()`, the event is triggered when MQTT is disconnected
      break;
    case HomieEventType::SENDING_STATISTICS:
      // Do whatever you want when statistics are sent in normal mode
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;
  Homie.disableLedFeedback();

  // Start Homie  
  Homie_setFirmware("TempSensor", "0.2.0");
  Homie.setLoopFunction(mainHomieLoop);
  Homie.onEvent(onHomieEvent); // before Homie.setup()
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
#ifdef DEBUG_EN
  Debug.handle();
#endif
}