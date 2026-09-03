# Portal UI

`WiFiManagerPortalConfig` is the supported presentation API for the built-in provisioning portal. It changes identity and semantic visual tokens only. WiFiManager continues to own portal routes, forms, navigation, captive behavior, reset, and OTA views.

Apply the configuration before `autoConnect()`, `startConfigPortal()`, or `startWebPortal()`. The configuration is non-owning: text and SVG data must have static firmware lifetime and declare whether it is in RAM or PROGMEM. WiFiManager locks presentation after a portal starts, so active asynchronous responses cannot observe a partial configuration.

## Standalone branded portal

```cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

namespace {
const char kTitle[] PROGMEM = "Set up Temperature Monitor";
const char kIdentity[] PROGMEM = "Example Devices";
const char kTagline[] PROGMEM = "Reliable setup for connected devices.";
const char kLogoAlt[] PROGMEM = "Example Devices";
const char kLogo[] PROGMEM = R"svg(<svg viewBox="0 0 64 64"><circle cx="32" cy="32" r="28"/></svg>)svg";
const char kPage[] PROGMEM = "#f4f7f3";
const char kSurface[] PROGMEM = "#ffffff";
const char kAccent[] PROGMEM = "#347a45";
const char kAccentText[] PROGMEM = "#ffffff";

const WiFiManagerPortalConfig kPortalUI = {
    WiFiManagerPortalText::progmem(kTitle),
    WiFiManagerPortalText::progmem(kIdentity),
    WiFiManagerPortalText::progmem(kTagline),
    WiFiManagerPortalAsset::svgFromProgmem(kLogo),
    WiFiManagerPortalText::progmem(kLogoAlt),
    {
        WiFiManagerPortalText::progmem(kPage),
        WiFiManagerPortalText::progmem(kSurface),
        {}, {}, {},
        WiFiManagerPortalText::progmem(kAccent),
        {},
        WiFiManagerPortalText::progmem(kAccentText),
        {}, {}, {},
        10, 6,
    },
};
}

void setup() {
    wifi.setPortalConfig(kPortalUI);
    wifi.autoConnect("Temperature Monitor");
}

void loop() {}
```

The complete buildable example is [BrandedPortal](../examples/BrandedPortal/BrandedPortal.ino). The compile fixture exercises this API on ESP8266 and ESP32.

## Configuration reference

Leave a text or colour value empty, or a radius at `0`, to retain the built-in stylesheet value. The default title is `WiFiManager`.

| Field | Used by |
| --- | --- |
| `title` | Document title and concise setup-page heading |
| `identityText` | Company or product name in the header above navigation |
| `tagline` | Short tagline in the header above navigation |
| `logo.svg`, `logoAltText` | Optional trusted inline SVG and its accessible label |
| `pageBackground`, `surface`, `text`, `mutedText`, `border` | Portal surfaces and text |
| `accent`, `accentHover`, `accentText` | Primary links and actions |
| `success`, `danger`, `dangerHover` | Status and destructive actions |
| `cornerRadiusPx`, `smallCornerRadiusPx` | Card and compact-control corners, limited to 64 px |

Theme values accept only simple semantic CSS value syntax and are emitted once into a small `#wm-portal-theme` block. This is deliberately not a raw CSS or JavaScript injection API. An SVG is a trusted compiled firmware asset, never form, MQTT, or network input.

## Built-in portal capabilities

Presentation uses one configuration route: `setPortalConfig()`. Existing structured portal capabilities remain separate: `portalAddParameter()`, `portalAddInfoSection()`, `portalAddHomeCard()`, page visibility, and portal behavior configure documented built-in functionality rather than private markup. See [Portal API](PORTAL_API.md) and [Station profiles](STATION_PROFILES.md).

There is no arbitrary HTML shell, route replacement, navigation injection, raw stylesheet, or script hook. If a product needs a new portal capability, add a narrow WiFiManager contract and test it on both supported targets.


Back to the [documentation index](README.md) or [project overview](../README.md).
