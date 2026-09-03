# Branded Portal

This example uses the supported `WiFiManagerPortalConfig` presentation API to give the built-in portal a product name, company identity, tagline, inline SVG mark, and semantic colour tokens.

Flash the `esp8266` or `esp32` environment, join **Temperature Monitor**, and open `http://192.168.4.1/`. The visual changes come from static firmware data; WiFiManager still owns the portal routes, forms, validation, and captive-network behaviour.

Use only trusted compiled SVG data. Keep the backing strings static for the lifetime of the firmware, then call `setPortalConfig()` before opening a portal.

See [Portal UI](../../docs/PORTAL_UI.md) and the shared [example guide](../README.md).
