# Portal browser harness

This test fixture starts only the WiFiManager captive portal. It intentionally has no station credentials, MQTT configuration, DeviceFramework dependency, or application logic.

Use it through the repository runner so a secondary Wi-Fi adapter is explicitly selected and protected from becoming the host default route:

```bash
./tools/portal-hardware run \
  --platform esp8266 \
  --port /dev/serial/by-id/... \
  --client-interface wlx...
```

The ESP8266 portal SSID is `WM Contract ESP8266`; the ESP32 SSID is `WM Contract ESP32`. Both use `default1` exclusively for local development tests.

The runner cleans up only the temporary connection it creates on the named secondary interface. It refuses to run if that interface is the system default route.
