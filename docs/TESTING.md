# Testing

The clean-consumer check builds a project that declares only WiFiManager. It proves the package manifest resolves DFTE, ESPAsyncWebServer, and the correct ESP8266 or ESP32 TCP dependency without a sibling checkout or attached board. The runner removes a previous local package link before each
check, so dependency resolution uses the current manifest rather than a stale
`.pio` copy.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
```

## Example builds

```bash
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
```

CI runs the clean-consumer and example checks for pushes to the maintained branch and pull requests. It intentionally compiles only: attached boards, local network state, and browser installation are not CI requirements.

## Local hardware lifecycle tests

The existing Unity suite runs on a board without Wi-Fi credentials, MQTT, a DeviceFramework checkout, or a local profile. It verifies portal start and stop lifecycle recovery, scan-cache release, and a real asynchronous Wi-Fi scan.

Use a stable serial-by-id path rather than a changing /dev/ttyUSB number:

~~~bash
pio device list
./scripts/test.sh hardware --platform esp8266 --port /dev/serial/by-id/usb-...
./scripts/test.sh hardware --platform esp32 --port /dev/serial/by-id/usb-...
~~~

The runner uploads the test image, captures its normal-boot serial output with the repository Bash helper, requires Unity's Tests 0 Failures and OK summary, and prints WM_METRIC lifecycle measurements. It does not need or read an environment file. A failed capture is preserved under /tmp for inspection; a successful one is removed.

## Optional portal browser test

test/portal-harness is a deliberately tiny portal-only firmware. It is not an example or a consuming application. The browser runner flashes it to the selected board, joins the portal with an explicitly named secondary Wi-Fi adapter, checks root HTML and bootstrap JSON, exercises concurrent root/bootstrap responses, and verifies an asynchronous scan. When a compatible local Chromium binary is available, it also captures the portal and records browser-console output.

~~~bash
./tools/test-portal-browser.sh --platform esp8266 --port /dev/serial/by-id/usb-... --wifi-interface wlx74da385d4165 --output /tmp/wifimanager-browser-results
~~~

The runner refuses an interface that owns the host's default route. It creates a temporary NetworkManager connection with never-default before bringing it up, so portal traffic stays on the specified adapter and does not replace the host Internet route. The temporary connection is removed on exit. Use a non-default secondary adapter only.

Back to [documentation](README.md) · [project overview](../README.md).
