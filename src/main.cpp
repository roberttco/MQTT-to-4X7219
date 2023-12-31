#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "EspMQTTClient.h"

#define MAX_DEVICES 4

#define CLK_PIN D5  // or SCK
#define DATA_PIN D7 // or MOSI
#define CS_PIN D8   // or SS

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

const uint8_t CHAR_SPACING = 1;
uint8_t SCROLL_DELAY = 75;
uint8_t DEFAULT_INTENSITY = 5;
uint8_t DIM_INTENSITY = 2;

bool connectionEstablished = false;

// waiting breathing intensity change
unsigned long lastBreath = 0;
bool waiting = false;
int inhale = 1;
int mqtt_intensity = DEFAULT_INTENSITY;
int breath_intensity = DEFAULT_INTENSITY;

// display dim after a period
unsigned long dimDelayMs = 15000;
unsigned long dimTimerStart = 0;
int dimLevel = DIM_INTENSITY;
bool dimmed = false;

void printText(uint8_t modStart, uint8_t modEnd, const char *pMsg)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
  uint8_t state = 0;
  uint8_t curLen = 0;
  uint16_t showLen = 0;
  uint8_t cBuf[8];
  int16_t col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  char *p = (char *)pMsg;

  do // finite state machine to print the characters in the space available
  {
    switch (state)
    {
    case 0: // Load the next character from the font table
      // if we reached end of message, reset the message pointer
      if (*p == '\0')
      {
        showLen = col - (modEnd * COL_SIZE); // padding characters
        state = 2;
        break;
      }

      // retrieve the next character form the font file
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state++;
      // !! deliberately fall through to next state to start displaying

    case 1: // display the next part of the character
      mx.setColumn(col--, cBuf[curLen++]);

      // done with font character, now display the space between chars
      if (curLen == showLen)
      {
        showLen = CHAR_SPACING;
        state = 2;
      }
      break;

    case 2: // initialize state for displaying empty columns
      curLen = 0;
      state++;
      // fall through

    case 3: // display inter-character spacing or end of message padding (blank columns)
      mx.setColumn(col--, 0);
      curLen++;
      if (curLen == showLen)
        state = 0;
      break;

    default:
      col = -1; // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

EspMQTTClient client(
    "qsvMIMt8Fm6NV3",
    "UbKNUJakLBLpOh",
    "192.168.2.6",
    // "BROKER_USER",
    // "BROKER_PASSWORD",
    "MAX7219_DISPLAY", // Client name that uniquely identify your device
    24552              // The MQTT port, default to 1883. this line can be omitted
);

void setup()
{
  Serial.begin(115200);
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, DEFAULT_INTENSITY);

  // client.enableDebuggingMessages();
  client.enableHTTPWebUpdater("update", "update", "/"); // Activate the web updater, must be set before the first loop() call.

  printText(0, MAX_DEVICES - 1, "WiFi?");

  dimTimerStart = millis();
}

void onConnectionEstablished()
{
  client.subscribe("leddisplay/brightness", [](const String &payload)
                   {
                    Serial.printf("MQTT brightness = %s\n",payload.c_str());
                    // map intensity of 0-100% to 0-15 for the max7219 
                    int mapped = min<int>(abs(payload.toInt()),100) * 15 / 100;
                    mqtt_intensity = mapped;
                    mx.control(MD_MAX72XX::INTENSITY, mapped); });

  client.subscribe("leddisplay/dimdelay", [](const String &payload)
                   {
                    Serial.printf("MQTT dim delay = %s\n",payload.c_str());
                    dimDelayMs = min<int>(abs(payload.toInt()),3600) * 1000;
                   });

  client.subscribe("leddisplay/dimlevel", [](const String &payload)
                   {
                    Serial.printf("MQTT dim level = %s\n",payload.c_str());
                    dimLevel = min<int>(abs(payload.toInt()),15);
                   });   

  client.subscribe("leddisplay/message", [](const String &payload)
                   {
                     Serial.printf("MQTT message = %s\n", payload.c_str());

                     if (!payload.isEmpty())
                     {
                       waiting = false;
                       dimTimerStart = millis();
                       dimmed = false;
                       mx.control(MD_MAX72XX::INTENSITY, mqtt_intensity);
                     }

                     printText(0, MAX_DEVICES - 1, payload.c_str());
                   });

  char addr[8];
  sprintf(addr, "%d.%d", WiFi.localIP()[2], WiFi.localIP()[3]);
  printText(0, MAX_DEVICES - 1, addr);

  delay(3000);

  printText(0, MAX_DEVICES - 1, "waiting");

  waiting = true;
  connectionEstablished = true;
}

void loop()
{
  client.loop();

  if (waiting)
  {
    if (millis() - lastBreath > 50)
    {
      mx.control(MD_MAX72XX::INTENSITY, breath_intensity);

      if (breath_intensity <= 1)
      {
        inhale = 1;
        delay(750);
      }

      if (breath_intensity >= 15)
        inhale = -1;

      breath_intensity += inhale;

      lastBreath = millis();
    }
  }

  if (!waiting && !dimmed && (millis() - dimTimerStart > dimDelayMs))
  {
    Serial.println("dimming");
    dimmed = true;
    mx.control(MD_MAX72XX::INTENSITY, dimLevel);
  }
}