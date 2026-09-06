#include <Arduino.h>
#include <WiFiManager.h>

namespace {

#if defined(ESP8266)
constexpr char kPortalSsid[] = "WM Browser ESP8266";
#else
constexpr char kPortalSsid[] = "WM Browser ESP32";
#endif
constexpr char kPortalPassword[] = "default1";

WiFiManager wifi;

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(300);

    // This harness is deliberately portal-only: it has no saved station
    // credentials and never exercises a consuming application.
    wifi.setConfigPortalTimeout(0);
    wifi.setAPStaticIPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0));
    wifi.startConfigPortal(kPortalSsid, kPortalPassword);
}

void loop() {
    wifi.process();
}
