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

## Debug: WiFiManager logging and DFTE

### How DFTE logging is designed

DFTE keeps a **single global pointer** `deviceFrameworkTemplateEngineLogger` (see DFTE’s `DeviceFrameworkTemplateEngineDebug.h`). By default it is **`nullptr`**: every `DFTE_LOG_*` macro expands to a cheap null check and does nothing—no virtual calls, no strings, no flash cost. A host (sketch or library) implements `DeviceFrameworkTemplateEngineLogger` (virtual `error` / `warn` / `info` / `debug`) and calls `deviceFrameworkTemplateEngineEnableLogging(&myLogger)` once. Until then, DFTE is silent on the happy path; internal code mostly uses `DFTE_LOG_ERROR` and `DFTE_LOG_WARN` when something is wrong.

### WiFiManager today

- **`WM_LOG_LEVEL`**: compile-time default is **`0`** (silent). Set **`1`–`5`** for `Error` … `Trace` (see `WiFiManagerLogLevel` in `WiFiManagerLogLevel.h`). Example: `-DWM_LOG_LEVEL=5` in `platformio.ini` for full verbosity. Strip all log **call sites** from the binary with **`WM_NO_LOG`** (or legacy **`WM_NODEBUG`**). At runtime, **`setLogEnabled`**, **`setLogOutput(enabled, maxLevel)`**, **`_logPrefix`**, and **`_logPort`** gate output; use **`WM_DEBUG_PORT`** to redirect the default `Print` target. **`WiFiManagerLogSink`** receives structured **`WiFiManagerLogMessage`** (level, subsystem, full line).
- **`WM_DFTE_LOGGING`** (build flag, **off by default**): when defined, `WiFiManagerServer::setupTemplateEngine()` registers **`WiFiManagerDfteLogger`**, which forwards DFTE into **`WiFiManager::log(..., "WM/DFTE", ...)`**. It uses **`deviceFrameworkTemplateEngineEnableLogging(..., this)`** so **`disableLoggingForOwner(this)`** on shutdown does not clobber another owner. If DFTE logging was already enabled, WiFiManager does not replace it.

Use **`pio test -e esp8266_dfte_log --without-uploading --without-testing`** in this repo to compile the test firmware with **`WM_DFTE_LOGGING`** enabled.

### Design note

DFTE uses a small optional logger pointer; WiFiManager uses **`#ifndef WM_NO_LOG`** around log call sites so release builds can omit strings entirely. **`WiFiManagerLogSink`** lets hosts (e.g. **DeviceFramework**) map levels without parsing ad hoc prefixes.

## License

See [LICENSE](LICENSE).
