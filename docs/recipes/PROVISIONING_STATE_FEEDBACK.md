# Provisioning state feedback

A physical device should give useful feedback without relying on a captive-browser redirect. Existing consuming firmware uses WiFiManager state to make setup, recovery, and connected states visible through an LED, display, or local log.

## State model

| Product state | WiFiManager signal | Typical feedback |
| --- | --- | --- |
| Normal connected operation | Station/profile controller connected, no configuration portal | Normal indicator and application services. |
| Trying known networks | Profile status is attempting or switching | Short recovery indication; no claim that setup is active yet. |
| Setup portal active | getConfigPortalActive() is true or WM_EVENT_PORTAL_STARTED | Installer-oriented setup indication and instructions. |
| Portal submitted credentials | isConfigPortalConnectPending() is true | Progress indication. |
| Portal connection succeeded | didConfigPortalConnectSucceed() or WM_EVENT_PORTAL_CONNECT_SUCCESS | Connected confirmation; application decides when services start. |
| Portal connection failed | didConfigPortalConnectFail() or WM_EVENT_PORTAL_CONNECT_FAILED | Failure indication with retry/recovery policy. |
| Candidate connected but not stored | Station event plus storageSaveFailed | Warning: active connection may not survive restart. |

## Polling pattern

~~~cpp
void updateIndicator() {
    if (wifi.getConfigPortalActive()) {
        setIndicator(IndicatorState::Setup);
    } else if (wifi.isConfigPortalConnectPending()) {
        setIndicator(IndicatorState::Connecting);
    } else if (wifi.didConfigPortalConnectFail()) {
        setIndicator(IndicatorState::Offline);
    } else {
        setIndicator(IndicatorState::Normal);
    }
}
~~~

Call this from the application loop after wifi.process(). For profile mode, add getStationStatus() so the product can distinguish an active station recovery attempt from a portal session.

## Event pattern

Events reduce polling for transitions, but getters remain the authoritative state. Use setEventCallback() to record a transition or wake a product state machine, then read the relevant portal/profile status. Avoid performing long work in the callback; keep it suitable for the normal firmware loop.

## Operator-facing messages

Use getConfigPortalConnectMessage() and getWLStatusString() for concise local diagnostics. Never place Wi-Fi passwords, portal parameter values, or secrets in an operator display or remotely collected logs.

See [Observability](../OBSERVABILITY.md) for the complete event list and [Field installer provisioning](FIELD_INSTALLER_PROVISIONING.md) for timeout policy.
