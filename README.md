# WiFiManager

This repository is a **breaking fork** of upstream [`tzapu/WiFiManager`](https://github.com/tzapu/WiFiManager).
It is not a drop-in replacement for upstream behavior, APIs, templates, or portal customization patterns.

If you are evaluating this fork, assume that core web-portal architecture has changed and review the code before adopting it in an existing upstream-based project.

**This repository:** [alexhopeoconnor/WiFiManager](https://github.com/alexhopeoconnor/WiFiManager)  
**Upstream:** [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)

## Basic provisioning

For a standalone Arduino project, create one long-lived `WiFiManager` and call
`autoConnect` during setup. It first tries saved station credentials; when that
fails it starts the configuration portal with the supplied AP name and password.

```cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

void setup() {
    Serial.begin(115200);
    wifi.setConfigPortalTimeout(180);  // seconds; 0 leaves it open

    if (!wifi.autoConnect("Device Setup", "change-me")) {
        ESP.restart();
    }
}

void loop() {
    // Normal application work after WiFi is connected.
}
```

The AP password must meet the Wi-Fi password requirements. For applications
built on DeviceFramework, configure `setConfigDevicePassword(...)` instead:
DeviceFramework passes the same validated password to WiFiManager, Arduino OTA,
HTTP Basic authentication, and WebSerial.


## Breaking Changes

This fork intentionally modernizes and restructures the configuration portal.
Notable differences from upstream include:

- The portal now serves a **single HTML shell** from `GET /`.
- Client navigation is handled as a **SPA with hash routing**.
- Interactive behavior is exposed through **JSON APIs under `/api/...`** plus firmware upload at **`POST /u`**.
- Legacy multi-page portal routes and legacy root-template override paths have been removed.
- The portal rendering pipeline is built around **DFTE** instead of upstream's monolithic HTML string assembly.
- JSON endpoints are expected to be **data-first**, not derived from generated HTML fragments.

## Current Improvements And Modernization

This fork currently includes the following architectural improvements:

- A **single-shell portal architecture** with embedded bootstrap JSON and embedded application JS.
- A **SPA-based configuration UI** for WiFi setup, parameters, info views, device actions, and OTA flow.
- A **clean API surface** for WiFi scanning, WiFi save, parameters, info, status, restart, erase, portal exit, and captive-portal close behavior.
- **Captive portal redirect handling** retained while removing duplicate legacy UI architecture.
- **Data-first JSON generation** for portal APIs, including info/device/about data, instead of HTML-to-JSON parsing.
- **Capability-driven UI flags** in bootstrap/API payloads so features like info, update, erase, and action visibility can be controlled by backend state.
- **Portal bootstrap contract v2**: nested `brand`, `context`, `pages`, `actions`, `layout`, `extraHomeCards`; Wi-Fi meta params include `kind` (`field` | `html`) for first-class custom HTML parameters.
- **SPA-native feedback UX** using in-DOM dialog/toast behavior rather than page-based action flows.
- **Connect-on-save handoff**: when portal save triggers a station connect, `/api/wifi/connect-status` now reports a brief success state with `stationIp` and `redirectUrl` so the SPA can show the new address and redirect before the AP shuts down.
- **Request-scoped shell rendering**: the root portal page is built for each `GET /` from `WM_ROOT_SHELL_TEMPLATE` using a fresh placeholder registry. Shell inputs are `%PAGE_TITLE%`, `%STYLES%`, `%BOOTSTRAP_JSON%`, `%PORTAL_APP_JS%`, and `%PORTAL_APPEND_JS%` — filled in `WiFiManagerHandlers` from WiFiManager state and embedded assets (not from a server-wide template registry).
- **Customization via WiFiManager `portal*` APIs** (`portalSetBrandTitle`, `portalSetPageInfoVisible`, `portalSetLayoutParamsLocation`, `portalAddParameter`, asset hooks, etc.) and JSON under `/api/...`, not by exposing placeholder-registry mutation to consumers.
- A clearer separation between:
  - shell rendering (handlers + SPA bootstrap)
  - JSON API responses
  - captive portal behavior
  - OTA handling
- Updated tests focused on the **shell contract**, **bootstrap payloads**, and **API JSON shapes** rather than removed legacy portal pages.

## Portal Customization Boundary

Stable, supported portal customization is intentionally scoped:

- `portalSetBrand*` for title, intro text, and logo SVG, plus `portalSetContextIdentityText(...)` for the user-facing runtime identity string on the home view.
- `portalSetPage*`, `portalSetAction*`, `portalSetLayout*`, and `portalSetBehavior*` for built-in portal capabilities and runtime behavior.
- `portalAddParameter(...)` for first-class custom parameters, including raw HTML blocks inside parameter-rendering surfaces (`#/wifi` or `#/setup`).
- `portalAddInfoSection(...)` and `portalAddHomeCard(...)` for structured extra content rendered by the built-in SPA.
- `portalAppendCss(...)`, `portalOverrideCss(...)`, and `portalAppendJs(...)` for light theming and enhancement hooks.

Not part of the stable API:

- arbitrary HTML injection into home/info/nav/shell
- replacing built-in SPA routing or action flow
- depending on undocumented DOM IDs or route internals
- treating `include/templates/*` as a supported consumer override surface

Appended JS should enhance rather than replace the built-in SPA. The documented hook contract is:

- `wm:ready` with `detail.boot`
- `wm:view-changed` with `detail.route`

If a consumer needs custom live widgets, new primary navigation concepts, or new backend-to-frontend workflows, that is considered **fork territory** rather than portal customization.

## Portal Customization Example

```cpp
WiFiManager wm;

wm.portalSetBrandTitle("Solar Battery Monitor Setup");
wm.portalSetContextIdentityText("Solar Battery Monitor");
wm.portalSetBrandHomeIntro(
    "Connect your monitor to WiFi, then review battery and inverter settings."
);
wm.portalSetBrandLogoSvg(
    "<svg viewBox='0 0 24 24' aria-hidden='true'>"
    "<path d='M12 2L4 12h5l-1 10 8-10h-5z'></path>"
    "</svg>"
);

wm.portalSetPageInfoVisible(true);
wm.portalSetPageUpdateVisible(false);
wm.portalSetActionEraseVisible(false);
wm.portalSetActionRestartVisible(true);
wm.portalSetLayoutParamsLocation(PortalParamsLocation::SetupPage);

wm.portalSetBehaviorCaptivePortalEnabled(true);
wm.portalSetBehaviorConnectOnSave(true);
wm.portalSetBehaviorExitAllowed(true);

wm.portalSetFieldPasswordPlaceholderMode(PortalPasswordPlaceholderMode::Masked);
wm.portalSetFieldStaticIpVisibility(PortalFieldVisibility::Auto);
wm.portalSetFieldStaticDnsVisibility(PortalFieldVisibility::Auto);

WiFiManagerParameter mqttHost(
    "mqtt_host",
    "MQTT host",
    "broker.local",
    64,
    "placeholder='broker.local'"
);
wm.portalAddParameter(&mqttHost);

// Raw HTML remains first-class for custom parameters, but only inside the
// parameter-rendering surfaces (#/wifi or #/setup), not arbitrary portal regions.
WiFiManagerParameter gpsHelp(
    "<div class='wm-callout wm-callout--info'>"
    "<p>GPS options below apply only when a GPS module is connected.</p>"
    "</div>"
);
wm.portalAddParameter(&gpsHelp);

PortalInfoSection battery;
battery.id = "battery";
battery.title = "Battery";
battery.items.push_back({"soc", "State of charge", "84%"});
battery.items.push_back({"voltage", "Voltage", "13.2V"});
wm.portalAddInfoSection(battery);

PortalHomeCard solar;
solar.id = "solar";
solar.title = "Solar summary";
solar.kind = PortalHomeCardKind::KeyValue;
solar.items.push_back({"pv", "PV input", "420W"});
solar.items.push_back({"load", "Load", "180W"});
wm.portalAddHomeCard(solar);

wm.portalAppendCss(
    ".wm-brand-logo svg{width:40px;height:40px;display:block;}"
    ".wm-hero-intro{max-width:42ch;}"
);

wm.portalAppendJs(
    "document.addEventListener('wm:ready', function(e){"
    "  console.log('Portal booted', e.detail.boot);"
    "});"
);
```

## WiFi Connect Status API

When `portalSetBehaviorConnectOnSave(true)` is enabled, saving WiFi credentials queues a station join and the SPA polls `GET /api/wifi/connect-status`.

Response shape:

```json
{
  "state": "waiting | success | failed",
  "message": "human readable status",
  "wifiStatus": "WL_CONNECTED",
  "stationIp": "192.168.1.42",
  "redirectUrl": "http://192.168.1.42/"
}
```

Notes:

- `stationIp` and `redirectUrl` are present only after a successful station join.
- If the portal HTTP server is not on port `80`, `redirectUrl` includes the active port.
- On success, WiFiManager keeps the portal alive briefly so the client can read the success payload and navigate to the new device address before the captive AP is shut down.
- This improves the handoff on typical home networks, but it is not a universal guarantee: client captive-portal helpers, browser behavior, DHCP timing, and network isolation can still affect whether the redirect completes automatically.

## Dependencies

This fork depends on **DFTE** ([Device Framework Template Engine](https://github.com/alexhopeoconnor/DFTE)) and **ESP32Async/ESPAsyncWebServer**.

The published `library.json` resolves DFTE from the maintained, pinned
`v1.0.2` Git tag. No sibling checkout is required to consume WiFiManager.
When developing both libraries together, use a local `symlink://` or
`file://` dependency in your own ignored PlatformIO override.


## Installation

Use the release tag and list the asynchronous web/TCP packages at project level.
PlatformIO installs the manifest dependencies automatically, but its dependency
finder requires these headers to be direct project dependencies before it adds
their include paths:

```ini
[common]
lib_deps =
    WiFiManager=https://github.com/alexhopeoconnor/WiFiManager.git#v3.0.5
    ESP32Async/ESPAsyncWebServer@3.9.1

[env:esp8266]
extends = common
platform = espressif8266
board = d1_mini
framework = arduino
lib_deps =
    ${common.lib_deps}
    ESP32Async/ESPAsyncTCP@2.0.0

[env:esp32]
extends = common
lib_deps =
platform = espressif32
board = esp32dev
framework = arduino
    ${common.lib_deps}
    ESP32Async/AsyncTCP@3.4.9
```

The text after `#` is a Git ref. PlatformIO clones the repository and checks
out that tag; it does not download a GitHub Release asset. A tag gives a
reproducible library input. Use a local checkout only while actively changing
the library:

```ini
lib_deps =
    WiFiManager=file:///path/to/WiFiManager
```

## Tests and releases

The CI workflow compiles a minimal clean consumer project for both supported
targets. These commands need no connected board:

```bash
./scripts/compile-check.sh --platform esp8266
./scripts/compile-check.sh --platform esp32
```

Before publishing a version, update `library.json`, `CHANGELOG.md`, and this
README, run both checks, then create the annotated tag:

```bash
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Push the branch and tag. GitHub Actions validates the PlatformIO package again
and creates the GitHub Release from that tag.

## AI Assistance Notice

Parts of this codebase have been developed and refactored with the aid of AI coding agents under human direction and review.

## License

See [LICENSE](LICENSE).
