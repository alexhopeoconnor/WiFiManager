#include <Arduino.h>
#include <WiFiManager.h>

namespace {
const char kTitle[] PROGMEM = "Set up Compile Fixture";
const char kIdentity[] PROGMEM = "WiFiManager";
const char kTagline[] PROGMEM = "A branded portal compile check.";
const char kLogoAlt[] PROGMEM = "WiFiManager";
const char kLogo[] PROGMEM = "<svg viewBox='0 0 24 24'></svg>";
const char kPage[] PROGMEM = "#f4f7f3";
const char kSurface[] PROGMEM = "#ffffff";
const char kText[] PROGMEM = "#1c251e";
const char kMuted[] PROGMEM = "#607064";
const char kBorder[] PROGMEM = "#d6e0d7";
const char kAccent[] PROGMEM = "#347a45";
const char kAccentHover[] PROGMEM = "#245a32";
const char kAccentText[] PROGMEM = "#ffffff";

const WiFiManagerPortalConfig kPortalConfig = {
    WiFiManagerPortalText::progmem(kTitle),
    WiFiManagerPortalText::progmem(kIdentity),
    WiFiManagerPortalText::progmem(kTagline),
    WiFiManagerPortalAsset::svgFromProgmem(kLogo),
    WiFiManagerPortalText::progmem(kLogoAlt),
    {
        WiFiManagerPortalText::progmem(kPage),
        WiFiManagerPortalText::progmem(kSurface),
        WiFiManagerPortalText::progmem(kText),
        WiFiManagerPortalText::progmem(kMuted),
        WiFiManagerPortalText::progmem(kBorder),
        WiFiManagerPortalText::progmem(kAccent),
        WiFiManagerPortalText::progmem(kAccentHover),
        WiFiManagerPortalText::progmem(kAccentText),
        {}, {}, {}, 10, 6,
    },
};
}  // namespace

WiFiManager wifiManager;

void setup() {
    wifiManager.setPortalConfig(kPortalConfig);
    wifiManager.setConfigPortalTimeout(1);
}

void loop() {}
