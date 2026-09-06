# Observability

WiFiManager exposes connection and portal state so product firmware can provide useful local feedback. Use getters as the source of truth and treat callbacks as notifications.

A device framework consuming WiFiManager uses this pattern to distinguish normal operation, offline recovery, active setup, and a successful portal connection before it changes LED state or starts product services.

## Portal state

| API | Meaning |
| --- | --- |
| getConfigPortalActive() | A configuration AP/portal is currently active. |
| getWebPortalActive() | A manually started portal web service is active. |
| hasEnteredConfigPortal() | Setup has been entered at least once since boot. |
| getConfigPortalConnectState() | Idle, queued, waiting, success, or failed for the most recent portal connection attempt. |
| isConfigPortalConnectPending() | A concise check for queued or waiting connection work. |
| didConfigPortalConnectSucceed() / didConfigPortalConnectFail() | Result checks for the most recent portal submission. |
| getConfigPortalConnectStatus() | Platform Wi-Fi status for that attempt. |
| getConfigPortalConnectMessage() | Human-readable status; suitable for logs or local display. |
| getLastConxResult() / getWLStatusString() | Legacy and general Wi-Fi connection result helpers. |

Use these values to drive product feedback. Do not derive state by scraping portal HTML or assuming that an autoConnect() false return means the portal session failed.

## Events

setEventCallback() installs an optional notification hook:

~~~cpp
wifi.setEventCallback([](WiFiManager::wm_event_t event) {
    switch (event) {
        case WiFiManager::WM_EVENT_PORTAL_STARTED:
            setIndicator(IndicatorState::Setup);
            break;
        case WiFiManager::WM_EVENT_PORTAL_CONNECT_SUCCESS:
            setIndicator(IndicatorState::ConnectingComplete);
            break;
        case WiFiManager::WM_EVENT_STATION_LINK_LOST:
            setIndicator(IndicatorState::Offline);
            break;
        default:
            break;
    }
});
~~~

| Event | Product use |
| --- | --- |
| WM_EVENT_PORTAL_STARTED / WM_EVENT_PORTAL_STOPPED | Begin or end setup feedback. |
| WM_EVENT_PORTAL_CONNECT_QUEUED / START / SUCCESS / FAILED | Show progress and result for a portal-submitted network. |
| WM_EVENT_STATION_PROFILE_ATTEMPT / CONNECTED / FAILED | Track profile-controller progress. |
| WM_EVENT_STATION_LINK_LOST / BACKOFF | Show recovery behavior after an established connection drops. |
| WM_EVENT_STATION_PROFILES_CLEARED | Reconcile product state after profiles are cleared. |

Events are intentionally small and do not carry credentials or mutable request state. Read the relevant getter inside the callback when more detail is needed.

## Station-profile status

When using profile mode, getStationStatus() reports:

- state: idle, loading, attempting, switching, connected, backoff, or portal;
- activeSlot and attemptedSlot;
- configuredProfiles;
- wifiStatus and a human-readable message;
- whether the last connection was a candidate;
- whether the profile store could not save a successful candidate.

A store-save failure is operationally important: the device may be connected now but will not necessarily reconnect after a restart. Preserve that distinction in an LED, display, or operator log.

## Logging

WiFiManager logs to its configured Print output unless a WiFiManagerLogSink is supplied.

~~~cpp
wifi.setLogPrefix("[network] ");
wifi.setLogOutput(true, WiFiManagerLogLevel::Info);
~~~

WiFiManagerLogLevel ranges from Silent through Error, Warn, Info, Debug, and Trace. A custom WiFiManagerLogSink receives a WiFiManagerLogMessage instead of the Print output. Redact SSIDs and never log passwords, portal form values, or product secrets into a remotely collected log.

## Scan status

For nearby-network progress, use getScanState(), isScanRunning(), hasValidScanResults(), and getScanResults(); see [Network configuration](NETWORK_CONFIGURATION.md#scan-behavior). The portal API exposes a matching local scan-status representation for the built-in UI.

## Continue

- [Provisioning lifecycle](PROVISIONING_LIFECYCLE.md)
- [Station profiles](STATION_PROFILES.md)
- [API reference](API_REFERENCE.md)

Back to [documentation](README.md) · [project overview](../README.md).
