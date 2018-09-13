/*
 * The MIT License (MIT)
 * Copyright (c) 2015-2017 François GUILLIER <dev @ guillier . org>
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include "localconfig.h" // Empty file by default

/*
 * 
 * cm/teleinfo-049767880615/data/index {"value": "000073370"}
 * cm/teleinfo-049767880615/data/rms_current {"value": "001"}
 * cm/teleinfo-049767880615/data/apparent_power {"value": "00320"}
 */

// if teleinfo is not working after 60s then go to sleep for 5 minutes
#ifndef TIMEOUT_IF_NO_TELEINFO
#define TIMEOUT_IF_NO_TELEINFO 60000
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "DEMO_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "DEMO_PASSPHRASE"
#endif

#ifndef MQTT_TOPIC_BASE_DEBUG
#define MQTT_TOPIC_BASE_DEBUG "exp/teleinfo-esp8266-1/data/"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER MQTTserver(192, 168, 99, 99);
#endif

#ifndef UPDATE_URL
#define UPDATE_URL "http://192.168.99.99:8888/teleinfo_historique_mqtt_deepsleep.ino.bin"
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
String mqtt_topic_base_debug = MQTT_TOPIC_BASE_DEBUG;
IPAddress MQTT_SERVER;
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

#ifdef DEBUG
String debug_val;
#endif
String label;
String value;
char checksum1;
char checksum2;
String sn;
String meter_index;
String current;
String power;
unsigned char state = 0;

// Connect to Wifi and report IP address
void start_Wifi()
{
    #ifdef DEBUG
    Serial.println("Starting Wifi");
    #endif
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.begin(ssid, password);
    int counter = 0;
    while ((WiFi.status() != WL_CONNECTED) && (counter < 240))
    {
        delay(500);
        counter ++;
        #if defined(DEBUG)
        Serial.print(".");
        #endif
    }
    if (counter >= 240)
    {
        #ifdef DEBUG
        Serial.println("Connection Failed! Rebooting...");
        #endif
        ESP.restart();
    }
    #ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    #endif
}

void setup()
{
    Serial.begin(1200, SERIAL_7E1);

    #ifdef DEBUG
    Serial.println("starting");
    #endif

    start_Wifi();

    delay(1000);

    pinMode(D1, INPUT_PULLUP);
    if (digitalRead(D1) == 0)
    {
        #ifdef DEBUG
        Serial.println("Update attempt");
        #endif

        ESPhttpUpdate.update(UPDATE_URL, ESP.getSketchMD5());

        #ifdef DEBUG
        Serial.println("Continuing");
        #endif
    }

    mqtt_client.setServer(MQTTserver, 1883);
}

void publish (String topic, String payload)
{
    if (!mqtt_client.connected())
    {
        mqtt_client.connect("arduinoClient");
        #ifdef DEBUG
        Serial.println("MQTT Connect");
        #endif
    }
    
    mqtt_client.publish(topic.c_str(), payload.c_str());
}

void publish_value(String sn, String name, String value, bool number=false)
{
    if (value != "")
    {
        if (number)
            value = String(value.toInt()); // Double convertion to remove leading zeros etc...
        else
            value = "\"" + value + "\"";
        publish("cm/teleinfo-" + sn + "/data/" + name, "{\"value\": " + value + "}");
    }
}

void loop()
{
    if (Serial.available() > 0)
    {
        char c = Serial.read();
        switch (state)
        {
            // Start of Frame
            case 0:
                if (c == 0x02)
                {
                    state = 1;
                    sn = "";
                    meter_index = "";
                    #ifdef DEBUG
                    debug_val = "{";
                    #endif
                }
                break;

            // End of Frame
            case 6:
                if (c == 0x03)
                {
                    #ifdef DEBUG
                    publish(mqtt_topic_base_debug + "debug", debug_val + "\"firmware_md5\": \"" + ESP.getSketchMD5() + "\"}");
                    #endif
                    
                    if (sn != "")
                    {
                        publish_value(sn, "index", meter_index, true);
                        publish_value(sn, "rms_current", current, true);
                        publish_value(sn, "apparent_power", power, true);

                        #ifdef DEBUG
                        Serial.println("Going to sleep...");
                        #endif                       
                        ESP.deepSleep(65e6 - micros());  // ~ 60 secondes
                        // Wake-up is done by a hardware reset
                        #ifdef DEBUG
                        Serial.println("This should not be printed");
                        #endif
                    }
                    state = 0;
                    break;
                }
                // NO BREAK HERE !
                // If frame is not yet finished then we carry on with a new group

            // Start of Group
            case 1:
                if (c != 0x0A)
                {
                    #ifdef DEBUG
                    publish(mqtt_topic_base_debug + "error1", String(c));
                    #endif
                    state = 0;
                } else
                {
                    state = 2;
                    label = "";
                    checksum1 = 0;
                    value = "";
                }
                break;
  
            // Label
            case 2:
                checksum1 += c;
                if ((c == 0x20) || (c == 0x09))
                    state = 3;
                else
                    label += String(c);
                break;

            // Value
            case 3:
                if ((c == 0x20) || (c == 0x09))
                {
                    checksum2 += c;
                    state = 4;
                }
                else
                {
                    checksum1 += c;
                    value += String(c);
                }
                break;

            // Checksum
            case 4:
                checksum1 = (checksum1 & 0x3F) + 0x20;
                checksum2 = (checksum2 & 0x3F) + 0x20;
                if ((checksum1 != c) && (checksum2 != c))
                {
                    #ifdef DEBUG
                    publish(mqtt_topic_base_debug + "checksum", String(checksum1) + ' ' + String(checksum2) + ' ' + String(c) + ' ' + label + ' ' + value);
                    #endif
                    state = 0;
                } else
                    state = 5;
                break;

            // End of Group
            case 5:
                if (c == 0x0D)
                {
                    #ifdef DEBUG
                    debug_val += "\"" + label + "\": \"" + value + "\", ";
                    #endif
                    state = 6;
                    if (label == "ADCO")
                        sn = value;
                    if (label == "BASE")
                        meter_index = value;
                    if (label == "IINST")
                        current = value;
                    if (label == "PAPP")
                        power = value;
                }
                else
                {
                    #ifdef DEBUG
                    publish(mqtt_topic_base_debug + "error5", "");
                    #endif
                    state = 0;
                }
                break;
        }
    } else if (millis() > TIMEOUT_IF_NO_TELEINFO)
    {
        #ifdef DEBUG
        Serial.println("No reception from meter... Going to sleep...");
        #endif                       
        ESP.deepSleep(600e6 - micros());  // 600 secondes
    }
}
