# Development and releases

Released consumers use the public Git tag. While changing WiFiManager and a sibling dependency together, point an ignored local PlatformIO override at a `symlink://` or `file://` checkout rather than changing tracked application dependencies.

```ini
lib_deps =
    WiFiManager=symlink:///path/to/WiFiManager
```

## Target pins

WiFiManager uses one maintained ESP32 test lane:

| Lane | pioarduino platform | Purpose |
| --- | --- | --- |
| `esp32` | `55.03.311` / Arduino-ESP32 3.3.11 | maintained baseline |

This is a test-target policy, not a library-manifest dependency: a consuming
application chooses its own `platform` and must validate the complete
framework/toolchain stack. Do not let a shared global PlatformIO cache choose
framework metadata or a compiler implicitly, and do not override just the
toolchain to repair a cache mismatch. Each pioarduino platform owns its
matching framework, uploader, and compiler package set.

Core 3 Wi-Fi builds need the C++14, `SOC_WIFI_SUPPORTED`, and `Network/src`
settings in this repository's `platformio.ini`; keep those settings together
when adding an ESP32 environment. The portal OTA fixture and every guided
example use this 3.3.11 ESP32 baseline.

ESP8266 test environments pin framework commit `521ae60` for the upstream
Postmortem large-jump linker fix. The exact rationale and update rule are in
the shared [ESP8266 linker-workaround note](https://github.com/alexhopeoconnor/arduino-home-assistant/blob/main/docs/ESP8266-LINKER-WORKAROUND.md).
For the pioarduino release-to-Core mapping and cache-collision diagnosis, see
[DeviceFramework's toolchain guide](https://github.com/alexhopeoconnor/DeviceFramework/blob/main/docs/TOOLCHAINS.md).

`./scripts/test.sh` and `./tools/portal-hardware ota --platform esp32` place
the ESP32 A/B fixture,
in a dedicated PlatformIO Core/cache directory, defaulting to
`${XDG_CACHE_HOME:-$HOME/.cache}/wifimanager-platformio/core-3.3.11`. That
keeps pioarduino's package-form `esptool` and generated environment separate
from stale global `tool-esptoolpy` metadata. Override the location with
`WIFIMANAGER_PLATFORMIO_CORE_DIR`,
`WIFIMANAGER_PLATFORMIO_PACKAGES_DIR`, and
`WIFIMANAGER_PLATFORMIO_CACHE_DIR` when space belongs elsewhere. The first
first install is several GiB; reserve at least 4 GiB plus cache headroom. It is
persistent and is never cleared by normal test commands.

For a disposable cache investigation, point that variable at an exact temporary
directory, run the affected command, inspect the resolved graph, then remove
only that directory:

```bash
wm_pio_core="$(mktemp -d /tmp/wifimanager-pio-XXXXXX)"
WIFIMANAGER_PLATFORMIO_CORE_DIR="$wm_pio_core" \
  ./scripts/test.sh compile --platform esp32
rm -rf -- "$wm_pio_core"
```

Start a release with `bump-version.sh`. It updates package metadata and canonical installation snippets, then creates the changelog section. Replace its generated TODO with the release summary and update any behavioural documentation before running:

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh unity --platform esp8266
./scripts/test.sh unity --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/test.sh ota-fixtures --platform esp8266
./scripts/test.sh ota-fixtures --platform esp32
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

When a physical ESP8266 and ESP32 are available, include their local lifecycle tests in the release gate. These tests remain opt-in because they flash the selected board and use its actual radio:

~~~bash
./scripts/test.sh hardware --platform esp8266 --port /dev/serial/by-id/usb-...
./scripts/test.sh hardware --platform esp32 --port /dev/serial/by-id/usb-...
~~~

When a physical ESP8266 or ESP32 and a spare USB Wi-Fi adapter are available,
run the Docker portal test harness as an additional release-gate check. It is
opt-in because it flashes the selected board and temporarily joins its AP, but
it refuses the host default-route adapter and leaves Docker responsible only
for browser/API testing:

```bash
./tools/portal-hardware run --platform esp8266 --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165
```

From an SSH/headless shell, the portal command may ask once for scoped
NetworkManager sudo authorization before the selected board is erased. This is
host setup, not a test secret; never add a sudo value to an env file or run the
whole runner as root. See [Testing](TESTING.md#networkmanager-authorization)
for the direct/Polkit and scoped-sudo behavior.

See [Testing](TESTING.md#docker-portal-test-harness) for cleanup, artifacts, and
optional station handoff credentials.

Run the portal HTTP OTA A/B test harness separately when a spare adapter and 4 MB
test board are available. It erases the selected board's flash, serial-flashes
A, and uses the real browser update form to upload B; do not replace its
automatic-reboot assertion with a manual reset:

```bash
./tools/portal-hardware ota --platform esp8266 --port /dev/serial/by-id/usb-... \
  --client-interface wlx74da385d4165
```

See [Portal HTTP OTA A/B test harness](TESTING.md#portal-http-ota-ab-test-harness) for
the partition, artifact, final-board-state, and adapter rules.

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release using that version’s changelog section. The workflow does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).
