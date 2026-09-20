# Portal browser harness

This test fixture starts only the WiFiManager captive portal. It intentionally has no station credentials, MQTT configuration, DeviceFramework dependency, or application logic.

Use it through the repository runner so a secondary Wi-Fi adapter is explicitly selected and protected from becoming the host default route:

```bash
./tools/portal-hardware run \
  --platform esp8266 \
  --port /dev/serial/by-id/... \
  --client-interface wlx...
```

The ESP8266 portal SSID is `WM Test Harness ESP8266`; the ESP32 SSID is `WM Test Harness ESP32`. Both use `default1` exclusively for local development tests.

The runner cleans up only the temporary connection it creates on the named secondary interface. It refuses to run if that interface is the system default route.

NetworkManager authority is a host prerequisite, not a fixture secret. A GUI
Polkit session may authorize the adapter directly; a headless/SSH invocation
validates sudo before flashing, then elevates only the generated portal
connection actions. Leave the runner itself unprivileged so its private state
and browser artifacts remain owned by the developer. See
[`docs/TESTING.md`](../../docs/TESTING.md#networkmanager-authorization) for
the `WM_NMCLI_AUTH` options.

## A/B portal OTA fixture

The physical portal HTTP OTA test harness uses one `*_ota` environment per
platform. It writes a harness-only A/B identity header to an ignored, owner-only
directory unique to that run before each build, so PlatformIO reuses dependency
objects while the fixture-only `/api/test/firmware-marker` endpoint still proves
the newly booted image. The test harness requires the real form's successful response, its automatic
restart, and two fresh B-marker responses. Passive serial capture is retained
for failure diagnosis, but a product log-message wording change cannot turn a
successful A-to-B update into a failed test.

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
portal-only fixture. It requires Python PySerial and retains a passive,
no-reset `serial-ota.log` in the private run artifact directory for both
success and failure diagnosis.
