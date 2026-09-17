# Testing

WiFiManager separates repeatable board-free builds from opt-in tests that flash
a real board or join a captive portal. The normal commands never need a board,
local Wi-Fi credentials, browser binary, or sibling checkout.

| Physical test harness | Transport | Host adapter | Secret source | Required proof |
| --- | --- | --- | --- | --- |
| Portal lifecycle suite | Serial flash + captive-portal HTTP/browser | Named secondary adapter | safe fixture AP password | Unity/lifecycle checks and portal UI/API coverage |
| Portal HTTP OTA | WiFiManager multipart `POST /u` | Named secondary adapter | safe fixture AP password | serial A → updater accepts/completes B → serial B, plus browser automatic-reboot/B-twice proof |

The selected secondary adapter is intentionally never used for normal LAN
testing. It is `never-default`, so the host's ordinary route remains intact.

## Board-free fixture, consumer, and example builds

The Unity compile check builds WiFiManager's own fixture without a board. The
consumer check builds a project that declares only WiFiManager, proving that a
normal PlatformIO dependency resolution can compile DFTE, ESPAsyncWebServer,
and the correct ESP8266 or ESP32 TCP dependency. Normal commands reuse the
persistent PlatformIO cache; they do not delete, reinstall, or separately
assert the package graph.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh unity --platform esp8266
./scripts/test.sh unity --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/test.sh ota-fixtures --platform esp8266
./scripts/test.sh ota-fixtures --platform esp32
```

CI runs these board-free checks for pull requests and pushes to the maintained
branch. `esp32` uses Arduino-ESP32 3.3.11 and compiles the guided examples.
The OTA fixture builds compile both immutable A
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
portal test harness and DeviceFramework's hardware runners on the same host, so
two first-party invocations cannot flash or use the same board at once.

## Docker portal test harness

`test/portal-harness` is deliberately tiny portal-only firmware, not an example
or consuming application. `tools/portal-hardware` flashes it to one explicitly
selected board, joins its AP through one explicitly selected **secondary**
Wi-Fi adapter, then runs its HTTP and browser test harness in a pinned Playwright
Docker image. Docker uses host networking only to reach the already-routed
portal; it never runs NetworkManager or changes host adapters.

The runner resolves PlatformIO from `WIFIMANAGER_PIO_EXECUTABLE`, then `PATH`,
then PlatformIO's standard `~/.platformio/penv/bin/pio` installation. That
makes the same command work from a non-interactive SSH shell without modifying
the user's `PATH`.

### NetworkManager authorization

The portal adapter is a host-side resource, separate from fixture credentials.
The runner never reads a sudo password from `test/.env`, an environment file,
or source control. `doctor` reports whether the current session can use
NetworkManager directly or will need scoped sudo. In a graphical desktop,
Polkit normally authorizes the selected adapter directly. In an SSH or other
headless session with no Polkit agent, the runner visibly validates `sudo -v`
before it erases or flashes the board, then uses `sudo -n nmcli` only to scan,
disconnect, join, and remove its generated connection on the named secondary
adapter.

The normal setting is `WM_NMCLI_AUTH=auto`. Use `WM_NMCLI_AUTH=sudo` to choose
the same scoped path deliberately, or `WM_NMCLI_AUTH=direct` only when a
working Polkit policy already grants the required actions. Do not run the whole
runner under `sudo`: its state files and browser artifacts intentionally remain
owned by the invoking developer. If the sudo ticket expires during a long run,
the runner stops with an actionable message rather than silently treating an
unauthorized rescan as a missing portal SSID. `down` uses the same scoped path
to remove a retained connection.

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
client may reconnect during that interval; the test harness deliberately retries
that transport interruption and still requires a reachable portal with a
complete, valid scan result.

The normal browser test harness catches the common regression case. When changing
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
credentials, and pass it explicitly. The runner parses only `WIFI_SSID` and
`WIFI_PASSWORD` into a generated mode-600 two-key file, mounts that file
read-only into the test container, and removes it after the browser run; it
never mounts the complete local environment file or logs either value. Because
browser traces can retain request bodies, this opt-in mode disables Playwright
screenshots, video, and tracing, including explicit diagnostic screenshots. It
cannot be combined with README-media capture. Docker builds from the tracked
`tests/portal-harness` directory only, so neither the source environment file
nor the generated two-key file enters its build context. Keep its private output
directory private and review any remaining report before sharing it.

After either a passing or failing station-handoff attempt, the runner
serial-flashes the portal-only fixture once more. Its `setup()` clears saved
station settings, so the selected test board returns to the clean no-station
portal state and does not retain the developer's Wi-Fi credentials. A failed
restore or a failure to see the cleaned fixture AP return makes the command
fail. `--keep` affects only the runner's temporary
secondary-adapter connection; it does not retain station credentials on the
board.

A retained session is deliberately never overwritten. Before touching
NetworkManager, the runner atomically records its uniquely generated connection
name; after creation it atomically replaces that pending record with the exact
UUID. Ordinary failures and interrupts remove that connection immediately, and
`down` accepts either record after an uncatchable host termination or an
intentional `up`/`--keep` session. It never removes another NetworkManager
connection.

```bash
cp test/portal-station.env.example test/portal-station.env
./tools/portal-hardware run ... --station-env test/portal-station.env
```

## Refresh README media

README media is an explicit ESP32-only capture, not part of normal testing or
CI. It uses the same real-board portal test harness above, but records a short
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
README tour is readable; it does not change normal browser-test-harness timing.
ESP8266 remains covered by the normal hardware and browser test harness but does
not produce duplicate README media.

## Portal HTTP OTA A/B test harness

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
normal portal test harness: it must be the explicitly named secondary adapter and
cannot be the host default-route interface. The test never attaches that
adapter to a normal station network. The fixture AP uses the safe local
`default1` WPA password; this is an AP-access test, not a claim that `/u` has
HTTP route authentication.

The test harness performs the following complete run:

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

The OTA command additionally requires Python with PySerial (the
`python3-serial` package on Debian/Ubuntu) and retains a passive,
no-reset `serial-ota.log` beside the browser artifacts. It attaches immediately
after serial-flashing A releases the port—before portal association and the A
marker check—and remains attached through the two B checks. A passing run
requires the log's ordered immutable A marker, WiFiManager's update-start and
update-complete lines, then immutable B marker. This preserves firmware-side
portal-start and DHCP evidence as well as OTA evidence, without manufacturing a
reset. OTA-only fixture images wait five seconds after their upload reset so
the passive recorder can attach before A/B boot evidence is emitted; ordinary
portal test-harness startup remains fast.

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
