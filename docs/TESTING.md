# Testing

WiFiManager separates repeatable package checks from opt-in tests that flash a
real board or join a captive portal. The normal commands never need a board,
local Wi-Fi credentials, browser binary, or sibling checkout.

## Clean consumer and example builds

The clean-consumer check builds a project that declares only WiFiManager. It
proves the package manifest resolves DFTE, ESPAsyncWebServer, and the correct
ESP8266 or ESP32 TCP dependency without a sibling checkout. The runner removes
a prior local package link before each check, so dependency resolution uses the
current manifest rather than a stale `.pio` copy.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
```

CI runs these board-free checks for pull requests and pushes to the maintained
branch. It intentionally does not require attached hardware, a local network,
or Docker.

## Local hardware lifecycle tests

The Unity suite runs on a board without Wi-Fi credentials, MQTT, DeviceFramework,
or a local profile. It verifies portal start/stop recovery, scan-cache release,
and a real asynchronous Wi-Fi scan.

Use a stable serial-by-id path rather than a changing `/dev/ttyUSB` number:

```bash
pio device list
./scripts/test.sh hardware --platform esp8266 --port /dev/serial/by-id/usb-...
./scripts/test.sh hardware --platform esp32 --port /dev/serial/by-id/usb-...
```

The runner flashes the selected board, captures normal-boot serial output with
the repository Bash helper, requires Unity's `Tests 0 Failures` and `OK`
summary, and prints lifecycle metrics. Hardware work shares a lock with the
portal contract, so two invocations cannot flash or use the same board at once.

## Docker portal contract

`test/portal-harness` is deliberately tiny portal-only firmware, not an example
or consuming application. `tools/portal-hardware` flashes it to one explicitly
selected board, joins its AP through one explicitly selected **secondary**
Wi-Fi adapter, then runs its HTTP and browser contract in a pinned Playwright
Docker image. Docker uses host networking only to reach the already-routed
portal; it never runs NetworkManager or changes host adapters.

```bash
./tools/portal-hardware doctor --client-interface wlx74da385d4165
./tools/portal-hardware run \
  --platform esp8266 \
  --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165
```

The command refuses the host default-route adapter. If the chosen secondary
adapter is already connected, require an explicit acknowledgement before it is
replaced:

```bash
./tools/portal-hardware run ... --take-over-client-adapter
```

The fixture opens a 15-minute portal session, includes one harmless custom
parameter, and verifies root/bootstrap/info/status API responses, concurrent
low-priority requests, parameter persistence, timeout reset, an actual async
scan, a missing-route response, and desktop/mobile portal rendering with no
browser page errors. Screenshots, traces on failure, JSON results, and the HTML
report are saved under the printed XDG state-directory artifact path.

For interactive diagnosis, leave the temporary client connection up and remove
only that managed connection when finished:

```bash
./tools/portal-hardware up --platform esp32 --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165
./tools/portal-hardware down
```

An optional station handoff test is deliberately separate because it connects
the fixture to a real LAN. Copy the ignored template below, add local
credentials, and pass it explicitly; it is mounted read-only into the test
container and is never logged by the runner.

```bash
cp test/portal-station.env.example test/portal-station.env
./tools/portal-hardware run ... --station-env test/portal-station.env
```

Back to [documentation](README.md) · [project overview](../README.md).
