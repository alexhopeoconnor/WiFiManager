# Testing

The clean-consumer check builds a project that declares only WiFiManager. It proves the package manifest resolves DFTE, ESPAsyncWebServer, and the correct ESP8266 or ESP32 TCP dependency without a sibling checkout or attached board.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
```

CI runs both checks for pushes to the maintained branch and pull requests. They compile only; hardware portals remain a local integration concern for an application or DeviceFramework’s connected-device suite.

Back to [documentation](README.md) · [project overview](../README.md).
