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

## A/B portal OTA fixture

The same fixture has dedicated `*_ota_a` and `*_ota_b` PlatformIO environments
for the physical portal HTTP OTA contract. A and B differ only by a compiled
marker served from the fixture-only `/api/test/firmware-marker` endpoint. That
proves a B boot without trusting saved portal values, EEPROM, or a filename.

```bash
./tools/portal-hardware ota \
  --platform esp32 \
  --port /dev/serial/by-id/... \
  --client-interface wlx...
```

ESP8266 explicitly uses `eagle.flash.4m1m.ld`. ESP32 uses the tracked two-slot
`partitions/esp32_ota_4m_no_fs.csv` layout on the maintained Arduino-ESP32
3.3.11 fixture lane. Both are 4 MB layouts. The runner builds and preserves A
and B before it touches the board, validates the matching ESP32 slots (or the
live ESP8266 updater capacity), then erases the explicitly selected test board
before serial-flashing A. A successful run leaves B installed in the
portal-only fixture.
