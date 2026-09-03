# WiFiManager examples

Every directory below is a standalone PlatformIO project. Build it from the repository root or the example directory:

```bash
pio run -d examples/BasicPortal -e esp8266
pio run -d examples/BasicPortal -e esp8266 -t upload
pio device monitor -d examples/BasicPortal -e esp8266
```

Choose `esp32` for an ESP32 development board. The examples use the checked-out WiFiManager source, so they are also useful while developing this fork.

| Example | Start here when you want to… |
| --- | --- |
| [Basic Portal](BasicPortal/) | provision one board through the normal saved-network-or-portal flow |
| [Custom Portal Content](CustomPortalContent/) | add application settings and useful status cards to the built-in portal |
| [Branded Portal](BrandedPortal/) | change the portal’s identity, logo, and semantic visual theme |
| [Station Profiles](StationProfiles/) | remember a primary Wi-Fi network and one fallback in application-owned EEPROM storage |

The portal examples intentionally begin with no station credentials. On first boot, connect to the access point printed on serial and open `http://192.168.4.1/`.

Back to the [project overview](../README.md).
