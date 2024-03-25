/**
 * WiFiManagerParameter child class example
 */
#include "wifisetup.h"
#include "matrix.h"

#include "EspMQTTclient.h"

Settings sett;

MD_MAX72XX *mx;

bool connectionEstablished = false;
bool waiting = false;

// waiting breathing intensity change
unsigned long lastBreath = 0;

int inhale = 1;
int mqtt_intensity = DEFAULT_INTENSITY;
int breath_intensity = DEFAULT_INTENSITY;

// display dim after a period
unsigned long dimDelayMs = 15000;
unsigned long dimTimerStart = 0;
int dimLevel = DIM_INTENSITY;
bool dimmed = false;

EspMQTTClient *client;

void onConnectionEstablished()
{
    Serial.println("MQTT connection established");

    waiting = true;
    connectionEstablished = true;
}

bool settingUp = false;
void setup()
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    pinMode(SETUP_PIN, INPUT_PULLUP);

    pinMode(D1, OUTPUT);
    digitalWrite(D1, 0);

    Serial.begin(115200);
    EEPROM.begin(512);

    mx = new MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

    mx->begin();
    mx->control(MD_MAX72XX::INTENSITY, DEFAULT_INTENSITY);

    if (digitalRead(SETUP_PIN) == LOW)
    {
        printText(mx, 0, MAX_DEVICES - 1, "config");
        settingUp = true;
        SetupWiFi();
        delay(3000);
        ESP.restart();
    }
    else
    {
        Serial.println("WORK");

        // warning for example only, this will initialize empty memory into your vars
        // always init flash memory or add some checksum bits
        EEPROM.get(0, sett);
        Serial.println("Settings loaded");

        // connect to saved SSID
        WiFi.begin();

        // do smth
        Serial.print("MQTT Port: ");
        Serial.println(sett.mqtt_port, DEC);

        Serial.print("IP param: ");
        IPAddress ip(sett.mqtt_ip);
        Serial.println(ip);

        client = new EspMQTTClient("192.168.2.6", sett.mqtt_port, "MQTT2MAX7219");
        client->enableHTTPWebUpdater("update", "update", "/"); // Activate the web updater, must be set before the first loop() call.
        client->setOnConnectionEstablishedCallback(onConnectionEstablished);
        client->enableDebuggingMessages(true);

        // spin until connected
        int timeout = 600;
        printText(mx, 0, MAX_DEVICES - 1, "WiFi?");

        Serial.println("Waiting for MQTT connection.");

        while (!client->isMqttConnected())
        {
            client->loop();
            delay(100);
            timeout -= 10;
            if (timeout <= 0)
            {
                Serial.println("Resetting after 60 second connection timeout.");
                ESP.restart();
            }
        }

        Serial.println("Setting up MQTT subscriptions.");

        client->subscribe("leddisplay/brightness", [](const String &payload)
                          {
                             // map intensity of 0-100% to 0-15 for the max7219
                             mqtt_intensity = (int)(min<int>(abs(payload.toInt()), 100) * 15 / 100);;
                             mx->control(MD_MAX72XX::INTENSITY, mqtt_intensity);
                             Serial.printf("MQTT brightness = %d\n\r", mqtt_intensity); });

        Serial.println("leddisplay/brightness subscribed");

        client->subscribe("leddisplay/dimdelay", [](const String &payload)
                          { dimDelayMs = (unsigned long)(min<int>(abs(payload.toInt()), 3600) * 1000);
                          Serial.printf("MQTT dim delay (ms) = %lu\n\r", dimDelayMs); });

        Serial.println("leddisplay/dimdelay subscribed");

        client->subscribe("leddisplay/dimlevel", [](const String &payload)
                          {

                        dimLevel = min<int>(abs(payload.toInt()),15);
                        Serial.printf("MQTT dim level = %d\n\r",dimLevel); });

        Serial.println("leddisplay/dimlevel subscribed");

        client->subscribe("leddisplay/message", [](const String &payload)
                          {
                             Serial.printf("MQTT message = %s\n\r", payload.c_str());

                             if (!payload.isEmpty())
                             {
                                 waiting = false;
                                 dimTimerStart = millis();
                                 dimmed = false;
                                 mx->control(MD_MAX72XX::INTENSITY, mqtt_intensity);
                             }

                             printText(mx, 0, MAX_DEVICES - 1, payload.c_str()); });

        Serial.println("leddisplay/message subscribed");

        client->subscribe("leddisplay/restart", [](const String &payload)
                          {
                             Serial.printf("MQTT message = %s\n\r", payload.c_str());

                             if (!payload.isEmpty())
                             {
                                if (payload.toInt() == 1) {
                                    system_restart();
                                }
                             }

                             printText(mx, 0, MAX_DEVICES - 1, payload.c_str()); });

        Serial.println("leddisplay/restart subscribed");

        char addr[8];
        sprintf(addr, "%3d.%3d", WiFi.localIP()[2], WiFi.localIP()[3]);

        printText(mx, 0, MAX_DEVICES - 1, addr);

        Serial.println("Waiting 3 seconds");

        delay(3000); // display the address for 3 seconds

        printText(mx, 0, MAX_DEVICES - 1, "waiting");

        Serial.println("All done subscribing");

        dimTimerStart = millis();
        settingUp = false;
    }
}

void loop()
{
    if (settingUp)
        return; // skip over the rest of this stuff if in wifi setup mode

    client->loop();

    if (waiting)
    {
        if (millis() - lastBreath > 50)
        {
            mx->control(MD_MAX72XX::INTENSITY, breath_intensity);

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
        Serial.printf("Dimming to %i\n\r", dimLevel);
        dimmed = true;
        mx->control(MD_MAX72XX::INTENSITY, dimLevel);
    }
}