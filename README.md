# WiFiManager

WiFiManager is the maintained ESP8266/ESP32 configuration-portal fork used by DeviceFramework. It is a deliberate breaking fork of upstream [`tzapu/WiFiManager`](https://github.com/tzapu/WiFiManager), with a streamed single-page portal and typed JSON APIs rather than upstream’s legacy page/template model.

## Why use it

- **Single-shell portal:** one responsive SPA for WiFi, parameters, information, actions, and firmware update flow.
- **Data-first APIs:** portal state and actions are exposed under `/api/...`, not scraped from HTML.
- **Controlled portal UI:** semantic identity/theme values and structured portal APIs without exposing portal internals.
- **ESP8266 and ESP32:** clean PlatformIO consumers resolve the right asynchronous TCP transport automatically.

## Try it

```cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

void setup() {
    Serial.begin(115200);
    wifi.setConfigPortalTimeout(180);
    if (!wifi.autoConnect("Device Setup", "change-me")) {
        ESP.restart();
    }
}

void loop() {}
```

For a DeviceFramework device, configure the framework’s shared device password instead. DeviceFramework applies it consistently to the provisioning AP, OTA, HTTP Basic authentication, and WebSerial.

### Brand it

```cpp
const char kTitle[] PROGMEM = "Set up Temperature Monitor";
const char kBrand[] PROGMEM = "Example Devices";
const char kAccent[] PROGMEM = "#347a45";

const WiFiManagerPortalConfig kPortalUI = {
    WiFiManagerPortalText::progmem(kTitle),
    WiFiManagerPortalText::progmem(kBrand),
    {}, {}, {},
    {
        {}, {}, {}, {}, {},
        WiFiManagerPortalText::progmem(kAccent),
        {}, {}, {}, {}, {},
        10, 0,
    },
};

void setup() {
    wifi.setPortalConfig(kPortalUI);  // before the portal starts
    wifi.autoConnect("Device Setup");
}
```

The full standalone example, including a static SVG, is [BrandedPortal](examples/BrandedPortal/BrandedPortal.ino).

## Explore the portal

| Goal | Guide |
| --- | --- |
| Brand the portal or add structured built-in content | [Portal UI](docs/PORTAL_UI.md) |
| Configure primary/fallback station profiles | [Station profiles](docs/STATION_PROFILES.md) |
| Understand the JSON APIs and station-connect handoff | [Portal API](docs/PORTAL_API.md) |
| Build a clean consumer or work on this fork | [Testing](docs/TESTING.md) · [Development](docs/DEVELOPMENT.md) |

## Install

```ini
[common]
lib_deps =
    WiFiManager=https://github.com/alexhopeoconnor/WiFiManager.git#v3.1.0
```

The suffix after `#` is a Git ref. PlatformIO clones the repository and checks out that release tag; GitHub Release assets are unrelated. This repository is supported through PlatformIO. Use a local `symlink://` or `file://` dependency only while actively changing this fork.

## Development and releases

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/check-docs.sh
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Tagging repeats the board-free compile checks, validates the package, and creates a GitHub Release from the corresponding changelog section. It does not publish to the PlatformIO Registry or deploy firmware.

See the [documentation index](docs/README.md), [release history](CHANGELOG.md), and [licence](LICENSE).
