# WiFiManager

Espressif ESPx WiFi Connection manager with fallback web configuration portal

## ⚠️ Fork Notice

**This is a fork with breaking changes.** This fork has been modified and optimized specifically for PlatformIO usage.

**Repository:** [alexhopeoconnor/WiFiManager](https://github.com/alexhopeoconnor/WiFiManager)

**For the original, unmodified version of WiFiManager, please visit the upstream repository:**
- [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)

This fork may contain breaking changes and is not compatible with the upstream version. Use at your own risk.

## Installation

This fork is optimized for PlatformIO. Add to your `platformio.ini`:

```ini
[env:your_environment]
platform = espressif8266  ; or espressif32
board = d1_mini  ; your board
framework = arduino
lib_deps = 
    path = ../../../WiFiManager  ; Relative path to this library
```

## License

See [LICENSE](LICENSE) file for details.
