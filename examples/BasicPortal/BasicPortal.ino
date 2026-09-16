#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager portal;

void setup() {
    Serial.begin(115200);
    portal.setConfigPortalTimeout(180);  // Do not leave a first-boot setup AP open forever.

    // Returns true when saved station credentials connect; otherwise opens the portal.
    if (portal.autoConnect("WiFiManager Basic", "example-pass")) {
        Serial.println("Connected. Run your normal application here.");
    } else {
        Serial.println("Setup portal started at http://192.168.4.1/");
    }
}

void loop() {
    portal.process();  // Keeps DNS, HTTP, and station-recovery work responsive.
}
