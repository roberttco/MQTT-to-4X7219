#include "wifisetup.h"

bool SetupWiFi()
{
  Settings settings;

  // Button pressed
  Serial.println("SETUP");

  WiFiManager wm;
  wm.resetSettings();

  // clear out the settings
  settings.mqtt_ip = 0;
  settings.mqtt_port = 0;
  settings.flags = FLAGS_CLEAR;
  EEPROM.put(0, settings);

  IntParameter mport("int", "MQTT_PORT", settings.mqtt_port);
  IPAddress ip(settings.mqtt_ip);
  IPAddressParameter mip("ip", "MQTT IP", settings.mqtt_ip);

  wm.addParameter(&mip);
  wm.addParameter(&mport);

  // SSID & password parameters already included
  wm.startConfigPortal();

  settings.mqtt_port = mport.getValue();
  settings.flags |= FLAGS_MQTT_PORT_SET;

  Serial.print("MQTT Port: ");
  Serial.println(settings.mqtt_port, DEC);

  if (mip.getValue(ip))
  {
    settings.mqtt_ip = ip;
    settings.flags |= FLAGS_MQTT_IP_SET;
    Serial.print("IP param: ");
    Serial.println(ip);
  }
  else
  {
    Serial.println("Incorrect IP");
    return false;
  }

  EEPROM.put(0, settings);

  if (EEPROM.commit())
  {
    Serial.println("Settings saved");
  }
  else
  {
    Serial.println("EEPROM error");
    return false;
  }

  return true;
}
