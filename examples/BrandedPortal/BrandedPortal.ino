#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

namespace {
const char kPortalTitle[] PROGMEM = "Set up Temperature Monitor";
const char kPortalIdentity[] PROGMEM = "Example Devices";
const char kPortalTagline[] PROGMEM = "Reliable setup for connected devices.";
const char kPortalLogoAlt[] PROGMEM = "Example Devices";
const char kExampleLogo[] PROGMEM = "<svg viewBox='0 0 64 64' aria-hidden='true'><circle cx='32' cy='32' r='28' fill='#347a45'/></svg>";
const char kPage[] PROGMEM = "#f4f7f3";
const char kSurface[] PROGMEM = "#ffffff";
const char kText[] PROGMEM = "#1c251e";
const char kAccent[] PROGMEM = "#347a45";
const char kAccentText[] PROGMEM = "#ffffff";

WiFiManagerPortalConfig kPortalUI;
}

void setup() {
    Serial.begin(115200);

    // Brand the built-in portal; unassigned fields retain their default values.
    kPortalUI.title = WiFiManagerPortalText::progmem(kPortalTitle);
    kPortalUI.identityText = WiFiManagerPortalText::progmem(kPortalIdentity);
    kPortalUI.tagline = WiFiManagerPortalText::progmem(kPortalTagline);
    kPortalUI.logo = WiFiManagerPortalAsset::svgFromProgmem(kExampleLogo);
    kPortalUI.logoAltText = WiFiManagerPortalText::progmem(kPortalLogoAlt);

    // Override only the theme tokens that define this product's appearance.
    kPortalUI.theme.pageBackground = WiFiManagerPortalText::progmem(kPage);
    kPortalUI.theme.surface = WiFiManagerPortalText::progmem(kSurface);
    kPortalUI.theme.text = WiFiManagerPortalText::progmem(kText);
    kPortalUI.theme.accent = WiFiManagerPortalText::progmem(kAccent);
    kPortalUI.theme.accentText = WiFiManagerPortalText::progmem(kAccentText);
    kPortalUI.theme.cornerRadiusPx = 10;
    kPortalUI.theme.smallCornerRadiusPx = 6;

    if (!wifi.setPortalConfig(kPortalUI)) {
        Serial.println("Portal UI configuration was rejected");
    }
    wifi.setConfigPortalTimeout(180);
    wifi.autoConnect("Temperature Monitor");
}

void loop() { wifi.process(); }
