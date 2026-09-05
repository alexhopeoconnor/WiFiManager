# Getting started

WiFiManager owns station credential recovery and a temporary configuration portal. Keep one instance for the life of the application.

```cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

void setup() {
    Serial.begin(115200);
    wifi.setConfigPortalTimeout(180);
    wifi.autoConnect("Example Setup", "change-me");
}

void loop() {
    wifi.process();  // Keeps the portal responsive when setup is needed.
    // Normal work begins after autoConnect succeeds.
}
```

`autoConnect()` first tries stored station credentials. If that cannot connect, it starts the AP and portal with the supplied name and password, then returns `false`; call `process()` from `loop()` so that portal can run. Use a Wi-Fi-valid AP password.

## PlatformIO dependency

```ini
lib_deps =
    WiFiManager=https://github.com/alexhopeoconnor/WiFiManager.git#v3.2.1
```

The package includes the asynchronous web and TCP dependencies required by the selected ESP8266 or ESP32 target. Add WiFiManager as the application’s direct dependency; do not copy its internal dependency list into your project.

Next: build [Basic Portal](../examples/BasicPortal/), then explore [portal UI](PORTAL_UI.md) or [portal API](PORTAL_API.md).

Back to [documentation](README.md) · [project overview](../README.md).
