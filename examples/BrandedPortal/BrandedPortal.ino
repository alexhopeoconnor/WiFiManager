#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

namespace {
const char kPortalTitle[] PROGMEM = "Set up Temperature Monitor";
const char kPortalIdentity[] PROGMEM = "Example Devices";
const char kPortalIntro[] PROGMEM = "Connect this device to Wi-Fi.";
const char kPortalLogoAlt[] PROGMEM = "Example Devices";
const char kExampleLogo[] PROGMEM = "<svg viewBox='0 0 64 64' aria-hidden='true'><circle cx='32' cy='32' r='28' fill='#347a45'/></svg>";
const char kPage[] PROGMEM = "#f4f7f3";
const char kSurface[] PROGMEM = "#ffffff";
const char kText[] PROGMEM = "#1c251e";
const char kAccent[] PROGMEM = "#347a45";
const char kAccentText[] PROGMEM = "#ffffff";

const WiFiManagerPortalConfig kPortalUI = {
    WiFiManagerPortalText::progmem(kPortalTitle),
    WiFiManagerPortalText::progmem(kPortalIdentity),
    WiFiManagerPortalText::progmem(kPortalIntro),
    WiFiManagerPortalAsset::svgFromProgmem(kExampleLogo),
    WiFiManagerPortalText::progmem(kPortalLogoAlt),
    {
        WiFiManagerPortalText::progmem(kPage),
        WiFiManagerPortalText::progmem(kSurface),
        WiFiManagerPortalText::progmem(kText),
        {}, {},
        WiFiManagerPortalText::progmem(kAccent),
        {},
        WiFiManagerPortalText::progmem(kAccentText),
        {}, {}, {},
        10, 6,
    },
};
}

void setup() {
    Serial.begin(115200);

    if (!wifi.setPortalConfig(kPortalUI)) {
        Serial.println("Portal UI configuration was rejected");
    }
    wifi.setConfigPortalTimeout(180);
    if (!wifi.autoConnect("Temperature Monitor")) {
        ESP.restart();
    }
}

void loop() {}
