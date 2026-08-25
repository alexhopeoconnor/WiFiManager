# Changelog

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

