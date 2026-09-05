#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager portal;

void setup() {
    Serial.begin(115200);
    portal.setConfigPortalTimeout(180);

    if (portal.autoConnect("WiFiManager Basic", "example-pass")) {
        Serial.println("Connected. Run your normal application here.");
    } else {
        Serial.println("Setup portal started at http://192.168.4.1/");
    }
}

void loop() { portal.process(); }
