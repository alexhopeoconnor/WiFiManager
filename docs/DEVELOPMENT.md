# Development and releases

Released consumers use the public Git tag. While changing WiFiManager and a sibling dependency together, point an ignored local PlatformIO override at a `symlink://` or `file://` checkout rather than changing tracked application dependencies.

```ini
lib_deps =
    WiFiManager=symlink:///path/to/WiFiManager
```

The ESP32 environments pin the PlatformIO-compatible pioarduino 51.03.05
platform package, which packages official Arduino-ESP32 3.0.5. This avoids the
known six-second asynchronous scan failure in the older 2.0.17 framework. Core
3 also requires the `SOC_WIFI_SUPPORTED`, `Network/src`, and ESP8266-transport
ignore settings shown in this repository `platformio.ini`; keep those settings
when adding an ESP32 environment.

Start a release with `bump-version.sh`. It updates package metadata and canonical installation snippets, then creates the changelog section. Replace its generated TODO with the release summary and update any behavioural documentation before running:

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release using that version’s changelog section. The workflow does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).
