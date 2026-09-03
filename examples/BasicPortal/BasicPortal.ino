#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager portal;

void setup() {
    Serial.begin(115200);
    portal.setConfigPortalTimeout(180);

    portal.autoConnect("WiFiManager Basic", "example-pass");

    Serial.println("Connected. Run your normal application here.");
}

void loop() { portal.process(); }
