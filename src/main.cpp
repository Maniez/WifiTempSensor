#include <Homie.h>
#include <DHT.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <SPI.h>
#include <Ticker.h>
#include <RemoteDebug.h> //https://github.com/JoaoLopesF/RemoteDebug
#include <Esp.h>
#include <EEPROM.h>

#include <IR/IRremoteESP8266.h>
#include <IR/IRsend.h>
#include <IR/ir_Fujitsu.h>

// Defines
#define DHTPIN 5
#define DHTTYPE DHT22
#define MeasurementTimeInSeconds 300
#define measurementsPerInterval 5

#define DEBUG_EN

#ifdef DEBUG_EN
#define HOST_NAME "tempsensor_and_ac_irremote-0"
#endif

// Gloabal Variables
EspClass ESP_;
EEPROMClass EEPROM_;

DHT dht(DHTPIN, DHTTYPE);
Ticker measurmentTimer;

const uint16_t kIrLed = 4; // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRFujitsuAC ac(kIrLed, ARRAH2E);

volatile float t = 0;
volatile float h = 0;

volatile float calibartion = 0;

bool globalEnableMeasurment = true;

#ifdef DEBUG_EN
#define DEBUG_SLOTS 50
RemoteDebug Debug;

uint32_t mqtt_lost_counter = 0;
uint32_t mqtt_lost_reason[DEBUG_SLOTS] = {0};
uint32_t mqtt_lost_time[DEBUG_SLOTS] = {0};
uint32_t wifi_lost_counter = 0;
uint32_t wifi_lost_reason[DEBUG_SLOTS] = {0};
uint32_t wifi_lost_time[DEBUG_SLOTS] = {0};
#endif

// Nodes
HomieNode MeasurementNode("Measurements", "Measurements", "string");
HomieNode KlimaNode("Klima", "Klima", "string");

bool doMeasurement(void)
{
  static int i = 0;
  static uint32_t time = 0;

  bool finished = false;
  // Read five times all values
  if (i == 0)
  {
    t = dht.readTemperature();
    h = dht.readHumidity();
    time = millis();
    i++;
  }
  else if (i < measurementsPerInterval)
  {
    if ((time + 100) < millis())
    {
      t += dht.readTemperature();
      h += dht.readHumidity();
      time = millis();
      i++;
    }
    else if (time > millis())
    {
      time = millis();
    }
  }
  else if (i == measurementsPerInterval)
  {
    // Calculate the mean of all values
    t = t / measurementsPerInterval + calibartion;
    h = h / measurementsPerInterval;
    Serial << "Send Temperatur: " << t << endl;
    Serial << "Send Humidity: " << h << endl;
    i = 0;
    finished = true;
  }
  else
  {
    i = 0;
  }

  return finished;
}

void enableMeasurment(void)
{
  globalEnableMeasurment = true;
}

void mainHomieLoop(void)
{
  if (globalEnableMeasurment == true)
  {
    // Do measurement of Temp and humidty
    if (doMeasurement())
    {
      Serial << "Measurement done" << endl;
      MeasurementNode.setProperty("Temperatur").send(String(t));
      MeasurementNode.setProperty("Humidity").send(String(h));

      globalEnableMeasurment = false;
    }
  }
}

void printState()
{
  // Display the settings.
  Serial.println("Fujitsu A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char *ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < ac.getStateLength(); i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

bool setPowerHandler(const HomieRange &range, const String &value)
{
  if (value.equals("true"))
  {
    Serial.println("Turn On AC");
    ac.setCmd(kFujitsuAcCmdTurnOn);
  }
  else
  {
    Serial.println("Turn Off AC");
    ac.setCmd(kFujitsuAcCmdTurnOff);
  }
  ac.send();
  return true;
}

bool setTempHandler(const HomieRange &range, const String &value)
{
  Serial.println("Set: " + value + "°C");
  ac.setTemp(value.toInt());
  ac.send();
  printState();
  return true;
}

bool setSwingHandler(const HomieRange &range, const String &value)
{
  if (value.equals("true"))
  {
    Serial.println("Set: Swing On");
    ac.setSwing(kFujitsuAcSwingVert);
    printState();
  }
  else
  {
    Serial.println("Set: Swing Off");
    ac.setSwing(kFujitsuAcSwingOff);
  }
  ac.send();
  return true;
}

bool setFanHandler(const HomieRange &range, const String &value)
{
  Serial.println("Set: Fan " + value);
  ac.setFanSpeed(value.toInt());
  ac.send();
  return true;
}

bool setModeHandler(const HomieRange &range, const String &value)
{
  Serial.println("Set: Fan " + value);
  ac.setMode(value.toInt());
  ac.send();
  return true;
}

#ifdef DEBUG_EN

// Process commands from RemoteDebug

void processCmdRemoteDebug()
{

  String lastCmd = Debug.getLastCommand();

  if (lastCmd == "debug1")
  {

    // Print Debug variables
    EEPROM_.begin(1024);
    EEPROM_.get(0, mqtt_lost_counter);
    for (uint8_t i = 0; i < DEBUG_SLOTS; i++)
    {
      EEPROM_.get(4 + i * 8, mqtt_lost_reason[i]);
      EEPROM_.get(8 + i * 8, mqtt_lost_time[i]);
    }

    EEPROM_.get(404, wifi_lost_counter);
    for (uint8_t i = 0; i < DEBUG_SLOTS; i++)
    {
      EEPROM_.get(408 + i * 8, wifi_lost_reason[i]);
      EEPROM_.get(412 + i * 8, wifi_lost_time[i]);
    }

    EEPROM_.commit();
    EEPROM_.end();

    debugD("Wifi disconnected %d times", wifi_lost_counter);
    for (uint32_t i = 0; (i < wifi_lost_counter) && (i < DEBUG_SLOTS); i++)
    {
      debugD("Wifi disconnected reason for disconnect %d was: %d at time %x", i + 1, wifi_lost_reason[i], wifi_lost_time[i]);
    }

    debugD("MQTT disconnected %d times", mqtt_lost_counter);
    for (uint32_t i = 0; (i < mqtt_lost_counter) && (i < DEBUG_SLOTS); i++)
    {
      debugD("MQTT disconnected reason for disconnect %d was: %d at time %x", i + 1, mqtt_lost_reason[i], mqtt_lost_time[i]);
    }
  }
  else if (lastCmd == "debug2")
  {

    // Clear Debug variables
    EEPROM_.begin(1024);
    mqtt_lost_counter = 0;
    EEPROM_.put(0, mqtt_lost_counter);
    for (uint8_t i = 0; i < DEBUG_SLOTS; i++)
    {
      mqtt_lost_reason[i] = 0;
      EEPROM_.put(4 + i * 8, mqtt_lost_reason[i]);
      mqtt_lost_time[i] = 0;
      EEPROM_.put(8 + i * 8, mqtt_lost_time[i]);
    }

    wifi_lost_counter = 0;
    EEPROM_.put(404, wifi_lost_counter);
    for (uint8_t i = 0; i < DEBUG_SLOTS; i++)
    {
      wifi_lost_reason[i] = 0;
      EEPROM_.put(408 + i * 8, wifi_lost_reason[i]);
      wifi_lost_time[i] = 0;
      EEPROM_.put(412 + i * 8, wifi_lost_time[i]);
    }

    EEPROM_.commit();
    EEPROM_.end();
  }
}
#endif

void onHomieEvent(const HomieEvent &event)
{
  String helpCmd;
  switch (event.type)
  {
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
    Debug.showColors(true);   // Colors

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
    wifi_lost_time[wifi_lost_counter] = (Homie.isConnected() << 8) | (wifi_station_get_connect_status() << 4) | Homie.getMqttClient().connected();

    EEPROM_.begin(1024);
    EEPROM_.put(404, wifi_lost_counter);
    EEPROM_.put(408 + wifi_lost_counter * 8, wifi_lost_reason[wifi_lost_counter]);
    EEPROM_.put(412 + wifi_lost_counter * 8, wifi_lost_time[wifi_lost_counter]);
    EEPROM_.commit();
    EEPROM_.end();

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
    mqtt_lost_time[mqtt_lost_counter] = (Homie.isConnected() << 8) | (wifi_station_get_connect_status() << 4) | Homie.getMqttClient().connected();

    EEPROM_.begin(1024);
    EEPROM_.put(0, mqtt_lost_counter);
    EEPROM_.put(4 + mqtt_lost_counter * 8, mqtt_lost_reason[mqtt_lost_counter]);
    EEPROM_.put(8 + mqtt_lost_counter * 8, mqtt_lost_time[mqtt_lost_counter]);
    EEPROM_.commit();
    EEPROM_.end();

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

void setup()
{
  Serial.begin(115200);
  Serial << endl
         << endl;
  //Homie.disableLedFeedback();

  // Start Homie
  Homie_setFirmware("TempSensor", "0.2.0");
  Homie.setLoopFunction(mainHomieLoop);
  Homie.onEvent(onHomieEvent); // before Homie.setup()
  measurmentTimer.attach(MeasurementTimeInSeconds, enableMeasurment);
  dht.begin();
  ac.begin();

  MeasurementNode.advertise("Temperatur").setName("Temp").setRetained(true).setDatatype("float");
  MeasurementNode.advertise("Humidity").setName("Humidity").setRetained(true).setDatatype("float");

  KlimaNode.advertise("power").setName("Powertoggle").setRetained(true).setDatatype("boolean").settable(setPowerHandler);
  KlimaNode.advertise("temperatur").setName("Temperatur").setRetained(true).setDatatype("integer").settable(setTempHandler);
  KlimaNode.advertise("swing").setName("Swing").setRetained(true).setDatatype("boolean").settable(setSwingHandler);
  KlimaNode.advertise("fan").setName("Fan").setRetained(true).setDatatype("integer").settable(setFanHandler);
  KlimaNode.advertise("mode").setName("Mode").setRetained(true).setDatatype("integer").settable(setModeHandler);

  // Setting default state for A/C.
  // See `fujitsu_ac_remote_model_t` in `ir_Fujitsu.h` for a list of models.
  ac.setModel(ARRAH2E);
  ac.setSwing(kFujitsuAcSwingOff);
  ac.setMode(kFujitsuAcModeAuto);
  ac.setFanSpeed(kFujitsuAcFanQuiet);
  ac.setTemp(20); // 20C
  ac.setCmd(kFujitsuAcCmdTurnOn);

  Homie.setup();
  ArduinoOTA.onStart([]()
                     { Serial.println("Start"); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                         Serial.println("Auth Failed");
                       else if (error == OTA_BEGIN_ERROR)
                         Serial.println("Begin Failed");
                       else if (error == OTA_CONNECT_ERROR)
                         Serial.println("Connect Failed");
                       else if (error == OTA_RECEIVE_ERROR)
                         Serial.println("Receive Failed");
                       else if (error == OTA_END_ERROR)
                         Serial.println("End Failed");
                     });
  ArduinoOTA.begin();
}

void loop()
{
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  Homie.loop();
#ifdef DEBUG_EN
  Debug.handle();
#endif
}
