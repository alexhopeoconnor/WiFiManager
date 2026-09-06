# Network configuration

These APIs configure WiFiManager's AP, station, scan, and reconnection behavior. Set them during boot, before starting a connection or portal flow, so an installer sees one consistent configuration.

The current ESP8266 and ESP32 implementations apply AP static addressing through WiFi.softAPConfig() and station static addressing through WiFi.config(). Station static configuration is used by legacy connections, portal save-and-connect attempts, and the station-profile controller.

This page is an advanced deployment reference. Use these settings only when the local network and the product's operating policy require them; WiFiManager's defaults are appropriate for many devices.

## Access-point setup network

| API | Purpose |
| --- | --- |
| setWiFiSSIDPrefix(prefix) | Changes the prefix used by an automatically generated AP name. |
| setWiFiAPChannel(channel) | Selects the setup AP Wi-Fi channel. |
| setWiFiAPHidden(hidden) | Hides or advertises the setup AP SSID. |
| setAPStaticIPConfig(ip, gateway, subnet) | Sets the AP-side portal address configuration. |
| setHttpPort(port) | Uses a non-default portal HTTP port. |
| setHostname(name) | Sets the station/AP hostname as supported by the target. |
| getConfigPortalSSID() / getDefaultAPName() | Reports the actual/current setup AP name. |

A hidden AP can make field setup harder because an installer must enter the SSID manually. Treat it as a deployment decision, not a general security control. If setHttpPort() changes the default, publish the full portal address in the product's installation procedure.

## A fixed setup-portal address

Give the setup AP a fixed address when an installer procedure needs a stable local portal address:

~~~cpp
wifi.setAPStaticIPConfig(
    IPAddress(192, 168, 8, 1),
    IPAddress(192, 168, 8, 1),
    IPAddress(255, 255, 255, 0));
wifi.setWiFiAPChannel(6);
wifi.setWiFiAPHidden(false);
~~~

This is applied before WiFiManager starts its SoftAP. The usual portal address becomes http://192.168.8.1/. If setHttpPort() selects a non-default port, include that port in the installer instructions and expect the station handoff URL to include it too.

## Station address and credential behavior

Use a station static address only when the network owner has reserved and documented it:

~~~cpp
wifi.setSTAStaticIPConfig(
    IPAddress(192, 168, 20, 50),
    IPAddress(192, 168, 20, 1),
    IPAddress(255, 255, 255, 0),
    IPAddress(192, 168, 20, 1));
~~~

| API | Purpose |
| --- | --- |
| setSTAStaticIPConfig(ip, gateway, subnet[, dns]) | Supplies a station static address and optional DNS server. |
| setCleanConnect(enabled) | Disconnects before connecting; use when the product requires a fresh association. |
| setRestorePersistent(enabled) | Controls restoration of the platform Wi-Fi persistence preference. |
| setWiFiAutoReconnect(enabled) | Enables Wi-Fi auto-reconnect behavior. |
| setCountry(countryCode) | Applies the supported Wi-Fi country setting. |
| disconnect() | Disconnects without erasing persistent settings. |
| resetSettings() | Clears legacy saved Wi-Fi settings. |
| erase([optional]) | Erases Wi-Fi configuration and schedules the configured reset behavior. |

Static-IP entry fields can be configured independently of the network setting:

~~~cpp
wifi.portalSetFieldStaticIpVisibility(PortalFieldVisibility::Auto);
wifi.portalSetFieldStaticDnsVisibility(PortalFieldVisibility::Hidden);
~~~

Auto shows these fields when a static station configuration is already in use; Always makes them visible; Hidden suppresses them. Do not expose static network controls in a general user portal unless the installer is expected to manage those values.

## Scan behavior

WiFiManager's portal schedules asynchronous scans. Firmware that needs its own nearby-network status can request and inspect the same cached scan state:

~~~cpp
wifi.requestAsyncScan();

if (wifi.hasValidScanResults()) {
    for (const auto& network : wifi.getScanResults()) {
        Serial.printf("%s: %ld dBm\n", network.ssid.c_str(), network.rssi);
    }
}
~~~

| API | Purpose |
| --- | --- |
| requestAsyncScan(forceRefresh) | Queues a scan without blocking the application loop. |
| getScanSnapshot() / getScanRuntimeState() | Returns the complete scan lifecycle snapshot. |
| getScanState() / isScanRunning() / hasValidScanResults() | Provides concise status checks. |
| getScanResults() | Returns the cached visible network list. |
| setMinimumSignalQuality(quality) | Filters low-quality scan results. |
| setRemoveDuplicateAPs(enabled) | Controls duplicate SSID removal. |
| setScanDispPerc(enabled) | Uses percentage rather than quality icons in the portal. |
| getRSSIasQuality(rssi) | Converts RSSI for display. |

WiFiManager owns the scan cache. It keeps the cache and its allocation while a portal is active so nearby networks can be rendered without repeated allocation churn; it clears and releases that storage when the portal fully closes. Treat getScanResults() as a snapshot and do not retain references, iterators, or pointers across a new scan or portal shutdown.

## Connection and timeout policy

See [Provisioning lifecycle](PROVISIONING_LIFECYCLE.md) for setConfigPortalTimeout(), setConnectTimeout(), setConnectRetries(), setSaveConnectTimeout(), setSaveConnect(), and the portal-prefixed behavior equivalents. These settings define recovery behavior; they should reflect how long an on-site installer can reasonably work and how long the product can remain offline.

## Continue

- [Portal UI and configuration](PORTAL_UI.md)
- [Provisioning lifecycle](PROVISIONING_LIFECYCLE.md)
- [API reference](API_REFERENCE.md)

Back to [documentation](README.md) · [project overview](../README.md).
