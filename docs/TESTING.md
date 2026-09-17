# Testing

WiFiManager separates repeatable package checks from opt-in tests that flash a
real board or join a captive portal. The normal commands never need a board,
local Wi-Fi credentials, browser binary, or sibling checkout.

| Physical contract | Transport | Host adapter | Secret source | Required proof |
| --- | --- | --- | --- | --- |
| Portal lifecycle suite | Serial flash + captive-portal HTTP/browser | Named secondary adapter | safe fixture AP password | Unity/lifecycle checks and portal UI/API contract |
| Portal HTTP OTA | WiFiManager multipart `POST /u` | Named secondary adapter | safe fixture AP password | A → rendered browser upload B → automatic reboot → B twice |

The selected secondary adapter is intentionally never used for normal LAN
testing. It is `never-default`, so the host's ordinary route remains intact.

## Board-free fixture, consumer, and example builds

The Unity compile check builds WiFiManager's own fixture without a board. The
clean-consumer check builds a project that declares only WiFiManager. Together
they prove the package manifest resolves DFTE, ESPAsyncWebServer, and the correct
ESP8266 or ESP32 TCP dependency without a sibling checkout. The runner removes
a prior local package link before each check, so dependency resolution uses the
current manifest rather than a stale `.pio` copy.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh compile --platform esp32-current
./scripts/test.sh unity --platform esp8266
./scripts/test.sh unity --platform esp32
./scripts/test.sh unity --platform esp32-current
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/test.sh ota-fixtures --platform esp8266
./scripts/test.sh ota-fixtures --platform esp32-current
```

CI runs these board-free checks for pull requests and pushes to the maintained
branch. `esp32` remains the explicit Arduino-ESP32 3.0.5 compatibility lane;
`esp32-current` is the clean-consumer Arduino-ESP32 3.3.11 validation lane.
The OTA fixture builds also use 3.3.11 for ESP32 and compile both immutable A
and B images against their tracked OTA partition layout. CI rejects equal A/B
artifacts, an ESP32 image larger than either 0x1F0000-byte app slot, or a
partition-table edit that breaks the required two-slot/no-filesystem layout. These checks
intentionally do not require attached hardware, a local network, or Docker.

## Local hardware lifecycle tests

The Unity suite runs portal-only firmware with no Wi-Fi credentials, MQTT,
product application framework, or local profile. It verifies portal start/stop
recovery, scan-cache release, and a real asynchronous Wi-Fi scan.

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

The fixture opens a 15-minute portal session and includes thirteen harmless
custom parameters. It verifies root/bootstrap/info/status API responses,
concurrent low-priority requests, every custom field and value across repeated
API fetches, timeout reset, an actual async scan, a missing-route response, and
desktop/mobile portal rendering with no browser page errors. It also round-trips
a value containing apostrophes, quotes, backslashes, angle brackets, and an
ampersand through the rendered form and parameter-save API. Screenshots, traces
on failure, JSON results, and the HTML report are saved under the printed XDG
state-directory artifact path.

On ESP8266, an AP+STA scan can briefly move the radio off the AP channel. The
client may reconnect during that interval; the contract deliberately retries
that transport interruption and still requires a reachable portal with a
complete, valid scan result.

The normal browser contract catches the common regression case. When changing
parameter rendering, run the opt-in ESP8266 soak as well. It performs twelve
full browser renders and API fetches while the AP is active, asserting all
thirteen fields and their exact values on every pass. This targets the
memory-sensitive rendering failure reported upstream in issue #1787 without
making every ordinary hardware run unnecessarily long:

```bash
./tools/portal-hardware run \
  --platform esp8266 \
  --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165 \
  --custom-parameter-stress
```

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

A retained session is deliberately never overwritten. If a previous `up` or an
interrupted `run` left one behind, run `./tools/portal-hardware down` first;
that removes only the named temporary connection recorded by the tool.

```bash
cp test/portal-station.env.example test/portal-station.env
./tools/portal-hardware run ... --station-env test/portal-station.env
```

## Refresh README media

README media is an explicit ESP32-only capture, not part of normal testing or
CI. It uses the same real-board portal contract above, but records a short
browser tour and stores all candidate files under the ignored
`artifacts/readme-media/` directory by default:

```bash
./tools/portal-hardware run \
  --platform esp32 \
  --port /dev/serial/by-id/usb-... \
  --client-interface USB_WIFI_ADAPTER \
  --capture-readme-media
```

Review the printed artifact directory. To keep a run somewhere more convenient,
pass `--output DIRECTORY`. After review, promote only the approved PNG/GIF
files into tracked documentation assets:

```bash
./tools/promote-readme-media \
  --from artifacts/readme-media/TIMESTAMP-esp32 \
  --replace
./scripts/check-docs.sh
```

The Docker renderer validates the GIF duration. The promotion tool requires the
successful ESP32 media manifest, checks file types and size limits, and never
copies raw video, browser reports, traces, or arbitrary artifact files. The
renderer preserves the real recording but deliberately presents it at 1.25×
duration and 6 fps so the
README tour is readable; it does not change normal browser-contract timing.
ESP8266 remains covered by the normal hardware and browser contract but does
not produce duplicate README media.

## Portal HTTP OTA A/B contract

`portal-hardware ota` is a separate opt-in physical test for WiFiManager's
built-in HTTP update path. It exercises the rendered firmware-update page and
its real multipart `POST /u` request; it is not an ArduinoOTA/UDP test.

```bash
./tools/portal-hardware ota \
  --platform esp8266 \
  --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165
```

The selected `--client-interface` has exactly the same safety rules as the
normal portal contract: it must be the explicitly named secondary adapter and
cannot be the host default-route interface. The test never attaches that
adapter to a normal station network. The fixture AP uses the safe local
`default1` WPA password; this is an AP-access test, not a claim that `/u` has
HTTP route authentication.

The runner performs the following complete contract:

1. Builds immutable A and B fixture images. Their marker is compiled into the
   binary, not saved in WiFiManager settings or EEPROM.
2. Checks both ESP32 images against the explicit matching `app0`/`app1` slots;
   ESP8266 validates B after A has booted against the exact aligned capacity
   passed to `Update.begin()`.
3. Erases the explicitly selected test board's flash, then flashes A over
   serial and starts its captive portal.
4. Joins that portal only through the named secondary adapter and requires the
   A marker at `/api/test/firmware-marker`.
5. Mounts B read-only into the Playwright container, chooses it in the real
   `#wm-ota-file` browser input, and submits the rendered form.
6. Requires the real `POST /u` success response, an automatic portal outage,
   automatic restart, and two independent B-marker responses.

The fixture marker endpoint exists only in `test/portal-harness`; it is not a
WiFiManager library route or a product-firmware pattern. The test does not
issue a manual reset. A board which boots B only after intervention is a
failure, even if B later appears.

Both fixture images are built with explicit OTA-capable layouts:

| Platform | Fixture layout | Capacity check |
| --- | --- | --- |
| ESP8266 | `eagle.flash.4m1m.ld` | A's live `ESP.getFreeSketchSpace()` response |
| ESP32 | two `0x1F0000` A/B app slots, no filesystem | tracked CSV `app1` size |

These are 4 MB fixture layouts (`d1_mini` for ESP8266 and `esp32dev` for
ESP32). Do not run this command against a board with another flash size unless
its matching explicit A/B layout and capacity checks have been added first.

The final board state is firmware B in the portal-only fixture: it clears
saved station settings on every boot and leaves no developer Wi-Fi credential
on the device. By default the temporary NetworkManager connection is removed
when the test exits. Pass `--keep` only for interactive diagnosis, then run
`./tools/portal-hardware down` to remove that named temporary connection.

Back to [documentation](README.md) · [project overview](../README.md).
