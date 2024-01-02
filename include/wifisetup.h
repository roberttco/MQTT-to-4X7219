#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Arduino.h>
#include <EEPROM.h>

#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#define SETUP_PIN D2    // the pin to hold low to force a configuration

bool SetupWiFi();

typedef struct _settings
{
  int mqtt_port;
  uint32_t mqtt_ip;
} Settings;

class IPAddressParameter : public WiFiManagerParameter
{
public:
  IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
      : WiFiManagerParameter("")
  {
    init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
  }

  bool getValue(IPAddress &ip)
  {
    return ip.fromString(WiFiManagerParameter::getValue());
  }
};

class IntParameter : public WiFiManagerParameter
{
public:
  IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
      : WiFiManagerParameter("")
  {
    init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
  }

  long getValue()
  {
    return String(WiFiManagerParameter::getValue()).toInt();
  }
};

#endif // WIFI_SETUP_H