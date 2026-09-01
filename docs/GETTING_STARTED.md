# Getting started

WiFiManager owns station credential recovery and a temporary configuration portal. Keep one instance for the life of the application.

```cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

void setup() {
    Serial.begin(115200);
    wifi.setConfigPortalTimeout(180);
    if (!wifi.autoConnect("Example Setup", "change-me")) {
        ESP.restart();
    }
}

void loop() {
    // Normal work begins after autoConnect succeeds.
}
```

`autoConnect()` first tries stored station credentials. If that cannot connect, it starts the AP and portal with the supplied name and password. Use a Wi-Fi-valid AP password.

## PlatformIO dependency

```ini
lib_deps =
    WiFiManager=https://github.com/alexhopeoconnor/WiFiManager.git#v3.1.0
```

The package manifest resolves DFTE, ESPAsyncWebServer, and the correct TCP transport for ESP8266 or ESP32. A consuming project should not duplicate those dependencies unless it is deliberately testing an unreleased stack change.

## DeviceFramework applications

DeviceFramework creates and configures WiFiManager for its normal lifecycle. Use DeviceFramework’s persistent shared device-password API instead of separately configuring an AP, OTA, HTTP, and WebSerial password.

Next: [portal UI](PORTAL_UI.md) or [portal API](PORTAL_API.md).

Back to [documentation](README.md) · [project overview](../README.md).
