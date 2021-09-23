#include "Arduino.h"
#include <ESP8266WiFi.h>
// Required for LIGHT_SLEEP_T delay mode
extern "C"
{
#include "user_interface.h"
}

const char *ssid = "D-Lan";
const char *password = "K4tM4n!!";

//The setup function is called once at startup of the sketch
void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
  }

  Serial.println();
  Serial.println("Start device in normal mode!");

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback()
{
  Serial.println("Callback");
  Serial.flush();
}

#define LIGHT_WAKE_PIN D5
#define FPM_SLEEP_MAX_TIME 0xFFFFFFF

void loop()
{
  Serial.println("Enter light sleep mode");
  Serial.flush();

  // Here all the code to put con light sleep
  // the problem is that there is a bug on this
  // process
  //wifi_station_disconnect(); //not needed
  uint32_t sleep_time_in_ms = 10000;
  gpio_pin_wakeup_enable(GPIO_ID_PIN(LIGHT_WAKE_PIN), GPIO_PIN_INTR_HILEVEL);
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_set_wakeup_cb(callback);
  //wifi_fpm_do_sleep(sleep_time_in_ms * 1000);
  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
  delay(sleep_time_in_ms + 1);

  Serial.println("Exit light sleep mode");

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  wifi_set_sleep_type(NONE_SLEEP_T);
  delay(10000); //  Put the esp to sleep for 15s
}