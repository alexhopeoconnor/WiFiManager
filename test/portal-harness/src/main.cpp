#include <Arduino.h>
#include <WiFiManager.h>

namespace {

#if defined(ESP8266)
constexpr char kPortalSsid[] = "WM Contract ESP8266";
#else
constexpr char kPortalSsid[] = "WM Contract ESP32";
#endif
constexpr char kPortalPassword[] = "default1";

WiFiManager wifi;
WiFiManagerParameter kInstallationLabel(
    "installation_label", "Installation label", "Contract fixture", 32);

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(300);

    // The fixture intentionally has no station credentials. A finite window
    // exercises timeout reset without leaving a board in a permanent portal.
    wifi.setConfigPortalTimeout(15 * 60);
    wifi.setAPStaticIPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0));
    wifi.portalAddParameter(&kInstallationLabel);
    wifi.startConfigPortal(kPortalSsid, kPortalPassword);
}

void loop() {
    wifi.process();
}
