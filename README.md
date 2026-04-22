# WiFiManager

This repository is a **breaking fork** of upstream [`tzapu/WiFiManager`](https://github.com/tzapu/WiFiManager).
It is not a drop-in replacement for upstream behavior, APIs, templates, or portal customization patterns.

If you are evaluating this fork, assume that core web-portal architecture has changed and review the code before adopting it in an existing upstream-based project.

**This repository:** [alexhopeoconnor/WiFiManager](https://github.com/alexhopeoconnor/WiFiManager)  
**Upstream:** [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)

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
- **SPA-native feedback UX** using in-DOM dialog/toast behavior rather than page-based action flows.
- A **shell-scoped placeholder model** centered on `%PAGE_TITLE%`, `%STYLES%`, `%SCRIPTS%`, `%BOOTSTRAP_JSON%`, and `%PORTAL_APP_JS%`.
- A clearer separation between:
  - shell rendering
  - API responses
  - captive portal behavior
  - OTA handling
- Updated tests focused on the **shell contract**, **bootstrap payloads**, and **API JSON shapes** rather than removed legacy portal pages.

## Dependencies

This fork depends on **DFTE** ([Device Framework Template Engine](https://github.com/alexhopeoconnor/DFTE)) and **ESP32Async/ESPAsyncWebServer**.

For this repo's `platformio.ini` test setup, DFTE is expected as a sibling checkout:

```ini
lib_deps =
    ESP32Async/ESPAsyncWebServer@3.9.1
    symlink://../DFTE
```

## Installation

```ini
[env:your_environment]
platform = espressif8266   ; or espressif32
board = d1_mini            ; your board
framework = arduino
lib_deps =
    https://github.com/alexhopeoconnor/WiFiManager.git
```

Or a local path:

```ini
lib_deps =
    file:///path/to/WiFiManager
```

## AI Assistance Notice

Parts of this codebase have been developed and refactored with the aid of AI coding agents under human direction and review.

## License

See [LICENSE](LICENSE).
