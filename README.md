# WiFiManager

Espressif ESPx WiFi connection manager with a fallback web configuration portal.

## Fork notice

**This is a fork with breaking changes.** The web portal is built with **DFTE** ([Device Framework Template Engine](https://github.com/alexhopeoconnor/DFTE)): composable templates and chunked HTML responses instead of upstream’s monolithic string assembly.

**This repository:** [alexhopeoconnor/WiFiManager](https://github.com/alexhopeoconnor/WiFiManager)

**Upstream (original):** [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)

## Dependencies

**DFTE** is required and is declared in `library.json` (PlatformIO / `lib_deps` pick it up automatically). For **this repo’s** `platformio.ini` tests, a **sibling checkout** is expected:

```ini
lib_deps =
    ESP32Async/ESPAsyncWebServer@3.9.1
    symlink://../DFTE
```

Clone `DFTE` next to `WiFiManager` (same parent directory), or point `lib_deps` at the Git URL used in `library.json`.

Other pinned dependency: **ESP32Async/ESPAsyncWebServer** (see `library.json`).

## Installation (PlatformIO)

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

## HTML customization (DFTE)

- **Page shell and routes** stream through DFTE (`TemplateRenderer` + chunked responses). Per-request bundles use their own `PlaceholderRegistry` for dynamic sections; the server also keeps a **shared** registry for defaults (`%STYLES%`, `%SCRIPTS%`, `%PAGE_TITLE%`, `%SUBTITLE%`, `%MENU%`, `%STATUS%`, …).
- **`WM_TEMPLATE_REGISTRY_CAPACITY`** (default in `WiFiManagerServer.h`) sizes the shared registry; raise it if you register many custom placeholders.
- **Override the root document template:** define **`WM_CUSTOM_ROOT_TEMPLATE_HEADER`** to a **quoted** include path (see `lib/WiFiManager/include/templates/RootSelector.h`). By default it includes `Root.h` (`WM_ROOT_TEMPLATE`).
- **Advanced / tests:** `WiFiManagerServer::instance()` exposes the same hooks after the server exists; prefer the `WiFiManager` methods below for typical apps.

### `WiFiManager` API

```cpp
wm.registerTemplateSetupCallback([](PlaceholderRegistry& reg) {
  // reg.registerProgmemData(...), registerRamData, registerDynamicTemplate, etc.
});
wm.rebuildPlaceholderRegistry();  // optional: after changing callback while portal runs
```

`registerTemplateSetupCallback` may be called **before** `startConfigPortal` / `startWebPortal` (stored until the HTTP server is created). If the portal is **already** running, the callback is applied and the shared registry is **rebuilt** so changes take effect.

`rebuildPlaceholderRegistry(customizer)` forwards to the server when present: rebuilds defaults, then runs your optional one-shot customizer, or the registered setup callback if `customizer` is null.

### Content built as `String` / PROGMEM fragments

Some small HTML snippets (status messages, OTA blurb, help table, etc.) are still assembled in RAM or pulled from `HTML.h` and then injected into templates. That is **intentional**: not every line of markup needs its own placeholder or registry churn. Customize those via DFTE where it pays off (shell, branding, layout); keep trivial blobs as strings when appropriate.

## Debug: WiFiManager (`DEBUG_WM`) and DFTE logging

### How DFTE logging is designed

DFTE keeps a **single global pointer** `deviceFrameworkTemplateEngineLogger` (see DFTE’s `DeviceFrameworkTemplateEngineDebug.h`). By default it is **`nullptr`**: every `DFTE_LOG_*` macro expands to a cheap null check and does nothing—no virtual calls, no strings, no flash cost. A host (sketch or library) implements `DeviceFrameworkTemplateEngineLogger` (virtual `error` / `warn` / `info` / `debug`) and calls `deviceFrameworkTemplateEngineEnableLogging(&myLogger)` once. Until then, DFTE is silent on the happy path; internal code mostly uses `DFTE_LOG_ERROR` and `DFTE_LOG_WARN` when something is wrong.

### WiFiManager today

- **`DEBUG_WM`**: compile-time gated with **`WM_DEBUG_LEVEL`** (see commented `#define WM_DFTE_LOGGING` / `WM_DEBUG_LEVEL` in `WiFiManager.h`). At runtime, `_debug`, `_debugLevel`, `_debugPrefix`, and `_debugPort` control volume and destination (`Serial` unless **`WM_DEBUG_PORT`** is set). This is tuned for **firmware size** (strip verbose strings when the macro is absent) and familiar serial prefixes (`*wm:`).
- **`WM_DFTE_LOGGING`** (build flag, **off by default**): when defined, `WiFiManagerServer::setupTemplateEngine()` registers **`WiFiManagerDfteLogger`**, which forwards DFTE messages into **`DEBUG_WM`** with a `[DFTE]` prefix and WM levels (error → `WM_DEBUG_ERROR`, warn → `WM_DEBUG_NOTIFY`, info → `WM_DEBUG_VERBOSE`, debug → `WM_DEBUG_DEV`). If **`deviceFrameworkTemplateEngineIsLoggingEnabled()`** is already true (for example **DeviceFramework** installed `WebInterfaceTemplateEngineLogger` first), WiFiManager **does not replace** that sink; it only owns teardown when it installed its own. **`shutdownServer()`** disables logging and drops the adapter only when WiFiManager installed it—avoiding clobbering a host logger on portal shutdown.

Use **`pio test -e esp8266_dfte_log --without-uploading --without-testing`** in this repo to compile the test firmware with **`WM_DFTE_LOGGING`** enabled.

### Should WiFiManager’s logging be refactored to match DFTE’s hook style?

**Not as a big-bang rewrite.** DFTE’s model fits a **small library** with one pipeline: optional sink, zero cost when disabled. WiFiManager has **hundreds** of `DEBUG_WM` sites and relies on **`#ifdef WM_DEBUG_LEVEL`** to remove entire print paths from the binary. Replacing that with only a runtime pointer would either **regain flash/RAM** for the strings or require keeping both mechanisms.

A practical middle path (what we did here) is: **keep `DEBUG_WM` + `WM_DEBUG_LEVEL` for WM-native traces**, and **optionally bridge DFTE into the same stream** with `WM_DFTE_LOGGING`. If you later want a unified sink for *both*, a small internal `WiFiManagerLogSink` interface *behind* `DEBUG_WM` could forward to `Serial` or a custom `Print`—that is incremental and does not force every call site through a virtual function in release builds.

## License

See [LICENSE](LICENSE).
