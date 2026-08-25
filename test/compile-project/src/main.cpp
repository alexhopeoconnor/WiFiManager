#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifiManager;

void setup() {
    wifiManager.setConfigPortalTimeout(1);
}

void loop() {}
