# Provisioning lifecycle

Choose a connection flow that matches the device's operating policy, then call process() regularly for as long as WiFiManager is active. It is a cooperative service call, not a hard real-time guarantee: target Wi-Fi or DNS work can occasionally take longer than a typical loop iteration. The consuming firmware owns its product-service lifecycle and restart decision.

## Choose a flow

| Need | Preferred API | What it does |
| --- | --- | --- |
| One saved station network, with portal fallback | autoConnect() | Tries saved credentials and starts the configuration portal when the attempt cannot connect. |
| Primary plus fallback network with application-owned storage | setStationProfileStore() + startStationConnection() | Loads, selects, retries, and recovers a fixed two-profile station set. |
| Verify a received profile before retaining it | startStationCandidate() | Attempts a primary/fallback candidate in memory and saves it only after success. |
| Start setup under application control | startConfigPortal() | Starts an AP and captive configuration portal immediately. |
| Show the portal while station Wi-Fi is already available | startWebPortal() | Starts the portal web service without starting a configuration AP. |

The first two are the normal deployed-device flows. StartConfigPortal() and startWebPortal() are supported control APIs, but should be used only when the application has a clear policy for entering and leaving them.

## Basic recovery flow

~~~cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager wifi;

void setup() {
    Serial.begin(115200);
    wifi.setConfigPortalTimeout(180);

    if (wifi.autoConnect("Device Setup", "change-me")) {
        startNormalApplication();
    } else {
        Serial.println("Wi-Fi setup portal is active");
    }
}

void loop() {
    wifi.process();

    if (wifi.didConfigPortalConnectSucceed()) {
        // The application decides whether to start services or reboot.
        startNormalApplication();
    }
}
~~~

autoConnect() returns true when WiFiManager connected during that call. It returns false when it could not connect and has entered the portal path; false is not, by itself, an instruction to restart. Call process() every loop iteration while setup may be needed.

A portal timeout is in seconds. setConfigPortalTimeout(0), the default, leaves the portal open. A field-installed product may deliberately use a longer bounded window, such as 15 minutes, so an installer has time to complete setup without leaving an unattended portal indefinitely.

## Portal connection outcome

For a portal save-and-connect attempt, use these getters instead of inferring state from the rendered page:

| Getter | Meaning |
| --- | --- |
| getConfigPortalActive() | The configuration portal is currently running. |
| hasEnteredConfigPortal() | The portal has been entered at least once this runtime session. |
| isConfigPortalConnectPending() | A portal-submitted station connection is queued or waiting. |
| didConfigPortalConnectSucceed() / didConfigPortalConnectFail() | Result of the last portal-submitted connection attempt. |
| getConfigPortalConnectStatus() / getConfigPortalConnectMessage() | Platform Wi-Fi status and a human-readable result. |

The local portal UI obtains the same state through its documented local API. Do not parse portal HTML to determine connection state.

## Timing and policy

Set these before the flow starts:

| API | Use |
| --- | --- |
| setConfigPortalTimeout(seconds) | Limits a captive configuration session; setTimeout() is its deprecated alias. |
| setConnectTimeout(seconds) and setConnectRetries(count) | Bounds legacy automatic connection attempts. |
| setSaveConnectTimeout(seconds) | Bounds a portal save-and-connect attempt. |
| setSaveConnect(enabled) | Controls whether a normal portal save attempts a station connection. |
| setBreakAfterConfig(enabled) | Exits after a configuration submission even if the connection was unsuccessful. |
| setEnableConfigPortal(enabled) / setDisableConfigPortal(enabled) | Controls autoConnect() portal fallback and its post-save shutdown behavior. |
| setAPClientCheck(enabled) / setWebPortalClientCheck(enabled) | Controls whether AP/web-client activity affects the portal timeout. |

The portal-prefixed behavior methods provide the same configuration through the structured portal contract; prefer one vocabulary consistently in a product.

## Port ownership and clean handoff

Before beginning a portal on port 80, stop any application server that already owns that port. After a portal connection succeeds, wait for the application's own readiness requirements before starting its server again. A successful station connection does not automatically make an application-level service ready.

See [Local web-service handoff](recipes/LOCAL_WEB_SERVICE_HANDOFF.md) for the integration sequence.

## Continue

- [Station profiles](STATION_PROFILES.md)
- [Observability](OBSERVABILITY.md)
- [API reference](API_REFERENCE.md)

Back to [documentation](README.md) · [project overview](../README.md).
