# Development and releases

Released consumers use the public Git tag. While changing WiFiManager and a sibling dependency together, point an ignored local PlatformIO override at a `symlink://` or `file://` checkout rather than changing tracked application dependencies.

```ini
lib_deps =
    WiFiManager=symlink:///path/to/WiFiManager
```

## Target pins

The ESP32 test environments pin the pioarduino `51.03.05` platform package,
which selects Arduino-ESP32 3.0.5 / ESP-IDF 5.1.4+. This is a test-target
contract, not a library-manifest dependency: a consuming application chooses
its own `platform` and must validate the complete framework/toolchain stack.
Core 3 Wi-Fi builds need the C++14, `SOC_WIFI_SUPPORTED`, `Network/src`, and
ESP8266-transport ignore settings in this repository's `platformio.ini`; keep
those settings together when adding an ESP32 environment.

ESP8266 test environments pin framework commit `521ae60` for the upstream
Postmortem large-jump linker fix. The exact rationale and update rule are in
the shared [ESP8266 linker-workaround note](https://github.com/alexhopeoconnor/arduino-home-assistant/blob/main/docs/ESP8266-LINKER-WORKAROUND.md).
For the pioarduino release-to-Core mapping and the scoped repair for a stale
global PlatformIO tool package, see [DeviceFramework's toolchain guide](https://github.com/alexhopeoconnor/DeviceFramework/blob/main/docs/TOOLCHAINS.md).

Start a release with `bump-version.sh`. It updates package metadata and canonical installation snippets, then creates the changelog section. Replace its generated TODO with the release summary and update any behavioural documentation before running:

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

When a physical ESP8266 and ESP32 are available, include their local lifecycle tests in the release gate. These tests remain opt-in because they flash the selected board and use its actual radio:

~~~bash
./scripts/test.sh hardware --platform esp8266 --port /dev/serial/by-id/usb-...
./scripts/test.sh hardware --platform esp32 --port /dev/serial/by-id/usb-...
~~~

When a physical ESP8266 or ESP32 and a spare USB Wi-Fi adapter are available,
run the Docker portal contract as an additional release-gate check. It is
opt-in because it flashes the selected board and temporarily joins its AP, but
it refuses the host default-route adapter and leaves Docker responsible only
for browser/API testing:

```bash
./tools/portal-hardware run --platform esp8266 --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165
```

See [Testing](TESTING.md#docker-portal-contract) for cleanup, artifacts, and
optional station handoff credentials.

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release using that version’s changelog section. The workflow does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).
