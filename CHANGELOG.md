# Changelog

## 3.2.0

- Refine the device-hosted portal with a startup Wi-Fi scan, stable loading overlays, clearer connection progress, and a resettable configuration timeout.
- Keep a successful portal-to-station hand-off reachable until the browser acknowledges its redirect, with a bounded fallback for captive or headless clients.
- Rename the presentation field `homeIntro` to `tagline` so portal identity and wording are clearer; update portal bootstrap contract to v3.
- Add focused Basic Portal, Branded Portal, Custom Portal Content, and Station Profiles examples for ESP8266 and ESP32, with real-hardware README captures.
- Strengthen release CI with documentation and clean-consumer/example compilation checks for both supported targets.

## 3.1.0

- Add an opt-in primary/fallback station-profile controller with bounded failover, reconnection, a durable consumer-supplied store, and profile-aware portal APIs. DeviceFramework uses this to persist verified WiFi profiles transactionally.
- Add portable portal branding and presentation hooks, including theme-aware shell and template rendering, without requiring DeviceFramework.
- Pin ESP32 tests to the Arduino 3-compatible pioarduino platform release and resolve DFTE 1.1.0.

## 3.0.6

- Correct async PlatformIO dependency owners to the registry's canonical lowercase identity, so a clean consumer builds WiFiManager and its ESP8266/ESP32 transport dependencies without duplicating them in `lib_deps`. Remove the superseded include-path bridge.

## 3.0.5

- Add a library-owned PlatformIO bridge for the asynchronous web/TCP include paths required when WiFiManager is nested beneath DeviceFramework.

## 3.0.4

- Set the package's deep PlatformIO dependency-discovery mode so WiFiManager's asynchronous web/TCP children remain available when it is consumed through DeviceFramework.

## 3.0.3

- Declare the ESP8266 and ESP32 async TCP libraries explicitly in the PlatformIO package manifest so clean consumers resolve the headers required by the asynchronous portal.

## 3.0.2

- Update the pinned DFTE dependency to the iterator-lifecycle and configuration-safe 1.0.2 release.

## 3.0.1

- Redact WiFi, AP, portal-form, and custom-parameter values from diagnostic logs.

## 3.0.0

- Establish `device-framework` as the independently maintained canonical branch.
- Add safe default parameter construction and allocation-failure handling.
- Pin the DFTE dependency used by PlatformIO builds.

