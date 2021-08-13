#pragma once
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>

using namespace websockets;

namespace whclient
{

    typedef double (*cb)(double value);
    const char *wsurl = "wss://ws.newklio.com:6000"; //server adress and port
    const char def_fingerprint[] PROGMEM = "06 60 91 ab a9 8c 42 cf a6 ea 30 14 a9 e7 94 f5 60 92 2b 4d";
    WebsocketsClient ws;
    bool connection_opened = false;
    class source
    {
    private:
        int cur_time = 0;

    public:
        bool timer_enabled = false;
        char *sourceName;
        String mode;
        double value;
        cb CallBack;
        int interval;

        void set(char *name, uint8_t mode, cb callback, int interval = 1000)
        {
            this->sourceName = name;
            this->interval = interval;
            if (mode == OUTPUT)
            {
                this->mode = "OUTPUT";
                timer_enabled = false;
            }

            if (mode == INPUT)
            {
                this->mode = "INPUT";
                timer_enabled = true;
                cur_time = millis();
            };
            this->CallBack = callback;
        }

        void push(double value)
        {
            this->value = value;
            DynamicJsonDocument doc(256);
            doc["status"] = "Push";

            // Serial.print(this->sourceName);
            // Serial.println(this->value);

            JsonArray payload = doc.createNestedArray("payload");
            payload.add(this->sourceName);
            payload.add(this->value);
            String output;
            serializeJson(doc, output);
            ws.send(output);
        }

        void ticker()
        {
            if (timer_enabled)
            {
                int temp = millis() - this->interval;
                if (temp >= this->cur_time)
                {
                    cur_time = millis();
                    this->value = this->CallBack(this->value);
                    DynamicJsonDocument doc(256);
                    doc["status"] = "Push";

                    // Serial.print(this->sourceName);
                    // Serial.println(this->value);

                    JsonArray payload = doc.createNestedArray("payload");
                    payload.add(this->sourceName);
                    payload.add(this->value);
                    String output;
                    serializeJson(doc, output);
                    ws.send(output);
                }
            }
        };
    };

    source *SOURCES_PTR;
    int SOURCES_SIZE = 0;

    void wifiSetup(const String &ssid, const String &passwd, unsigned long baud = 9600)
    {
        Serial.begin(baud);
        WiFi.mode(WIFI_OFF);
        pinMode(D4, OUTPUT);
        delay(1000);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, passwd);
        Serial.println("");
        Serial.print("Connecting to ");
        Serial.println(ssid);

        while (WiFi.status() != WL_CONNECTED)
        {
            digitalWrite(D4, LOW);
            delay(500);
            Serial.print(".");
            digitalWrite(D4, HIGH);
            delay(500);
        }

        Serial.println("");
        Serial.println("Connected Successfully!");
        digitalWrite(D4, HIGH);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        String banner = "Team Wireless Home Welcomes You!...";
        Serial.println(banner);
    }

    void registerSources()
    {
        DynamicJsonDocument toSend(256);
        toSend["status"] = "Register";
        JsonArray payload = toSend.createNestedArray("payload");
        for (int i = 0; i < SOURCES_SIZE; i++)
        {
            // Serial.println(SOURCES_PTR[i].sourceName);
            payload.add(SOURCES_PTR[i].sourceName);
        }
        String output;
        serializeJson(toSend, output);
        // Serial.println(output);
        ws.send(output);
    }

    void onRegistered(JsonArray sources)
    {
        for (JsonArray s : sources)
        {
            const char *SN = s[0];
            const char *mode = s[1];
            double value = s[2];
            Serial.print("Reg-> ");
            Serial.print(SN);
            Serial.print(", ");
            Serial.print(mode);
            Serial.print(", ");
            Serial.println(value);
        }
    }

    void onUpdate(JsonArray source)
    {
        String SN = source[0];
        double value = source[1];
        Serial.print("Up-> ");
        Serial.print(SN);
        Serial.print(", ");
        Serial.println(value);

        for (int i = 0; i < SOURCES_SIZE; i++)
        {
            String name = SOURCES_PTR[i].sourceName;
            if (name == SN)
            {
                SOURCES_PTR[i].CallBack(value);
            }
        }
    }

    void onError(String message)
    {
        Serial.print("Error-> ");
        Serial.println(message);
    }

    void onMessageCallback(WebsocketsMessage message)
    {
        // Serial.print("Got Message: ");
        String data = message.data();
        // Serial.println(data);
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, data);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        // Serial.println(data);
        String event = doc["status"];
        // Serial.println(event);

        if (event == "Auth")
        {
            JsonObject payload = doc["payload"];
            String msg = payload["msg"];
            // Serial.println(msg);
            if (msg == "Authentication Failed!")
            {
                Serial.println("Authentication Failed!");
                ws.close();
                while (true)
                {
                    digitalWrite(D4, LOW);
                    delay(100);
                    digitalWrite(D4, HIGH);
                    delay(100);
                }
            }
            // for (int i=0; i<SOURCES_SIZE; i++) {
            //     Serial.println(SOURCES_PTR[i].sourceName);
            // }
            connection_opened = true;
            Serial.println("Authenticated Successfully!");
            registerSources();
        }

        if (event == "Register")
        {
            JsonArray payload = doc["payload"];
            onRegistered(payload);
        }

        if (event == "Update")
        {
            JsonArray payload = doc["payload"];
            onUpdate(payload);
        }

        if (event == "Error")
        {
            JsonObject payload = doc["payload"];
            String msg = payload["msg"];
            onError(msg);
        }
    }

    void onEventsCallback(WebsocketsEvent event, String data)
    {
        if (event == WebsocketsEvent::ConnectionOpened)
        {
            // Serial.println("Connnection Opened");
        }
        else if (event == WebsocketsEvent::ConnectionClosed)
        {
            connection_opened = false;
            Serial.println("Connnection Closed");
        }
        else if (event == WebsocketsEvent::GotPing)
        {
            Serial.println("Got a Ping!");
        }
        else if (event == WebsocketsEvent::GotPong)
        {
            Serial.println("Got a Pong!");
        }
    }

    template <size_t n>
    void begin(const String &key, const String &deviceID, source (&sources)[n], const char *fingerprint = def_fingerprint, const char *url = wsurl)
    {
        SOURCES_PTR = sources;
        SOURCES_SIZE = n;
        ws.addHeader("x-access-token", key);
        ws.addHeader("deviceID", deviceID);
        ws.onMessage(onMessageCallback);
        ws.onEvent(onEventsCallback);
        ws.setFingerprint(fingerprint);
        // Connect to server
        ws.connect(url);
    }

    void run()
    {
        ws.poll();

        if (connection_opened)
        {
            for (int i = 0; i < SOURCES_SIZE; i++)
            {
                SOURCES_PTR[i].ticker();
            }
        }
    }
} // namespace whclient
