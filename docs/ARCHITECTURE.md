# Architecture and boundaries

WiFiManager is a device-local provisioning component. It tries station Wi-Fi, temporarily hosts an access point and portal when the device cannot connect, and then returns control to the firmware. It is not a cloud service, a remote-management system, or a companion-app framework.

## Ownership

| WiFiManager owns | The application owns |
| --- | --- |
| Temporary AP, captive DNS, portal routes, Wi-Fi connection attempts, portal session state | Durable application settings, schema migration, product services, LEDs/display, reboot policy, telemetry, and access-control decisions |
| Wi-Fi credentials in the legacy flow, or profile-selection policy in station-profile mode | The durable station-profile store when profile mode is enabled |
| Structured portal presentation and built-in Wi-Fi/settings forms | Product-specific validation and the persistence of product settings |
| The local portal JSON protocol | Any separate product HTTP API or local web server |

This separation is intentional: an application can decide how a device should be configured without having to duplicate the portal.

## Typical boot sequence

~~~text
Firmware boot
  │
  ├─ Load and migrate application settings
  ├─ Register portal branding, policy, and application parameters
  ├─ Start a WiFiManager connection flow
  │    ├─ Connected: start normal application services
  │    └─ Not connected: temporary AP + local portal
  ├─ Service wifi.process() while a portal or profile controller is active
  └─ Persist application data and decide when to restart or resume services
~~~

Register portal configuration before calling autoConnect(), startConfigPortal(), startWebPortal(), or a station-profile start method. The active portal uses an immutable response model so that asynchronous requests cannot see a partially changed UI.

## Structured configuration, not a replacement web app

WiFiManager supports product identity, semantic theme values, page/action visibility, field policy, parameters, information sections, and home cards. It deliberately does not provide:

- arbitrary portal HTML-shell replacement;
- raw CSS or JavaScript injection;
- route replacement or navigation injection;
- a cloud API or a supported mobile-companion integration surface.

Use the portal APIs where their existing semantics fit. If a product needs a new portal capability, add a narrow, documented WiFiManager contract and test it on both supported targets instead of reaching into portal internals.

## Application-service handoff

WiFiManager's portal server uses its configured HTTP port, 80 by default. A product already using that port must explicitly release it before WiFiManager starts a portal. Conversely, the application decides when its normal web service is safe to start after a successful connection.

This is a real integration boundary, not a WiFiManager callback side effect:

~~~cpp
void beginRecovery() {
    stopApplicationWebServer();       // Releases port 80 owned by the product.
    wifi.startConfigPortal("Device Setup", "setup-password");
}
~~~

The application may instead use a different WiFiManager HTTP port through setHttpPort(); document that address for installers because the captive-portal redirect and station handoff will include the selected port.

## Data lifetime and persistence

WiFiManagerPortalConfig text and SVG values are non-owning. Keep RAM or PROGMEM source data alive for the entire firmware lifetime. WiFiManagerParameter instances are application-owned and must outlive the portal. Portal information sections and home cards are copied when registered.

A station-profile store is also application-owned. Its load(), save(), and clear() methods are responsible for durable storage and error handling. The profile controller chooses and verifies networks; it does not own the store or an application's migration format.

## Security and operator expectations

The configuration portal is intended for local setup. Use a Wi-Fi-valid AP password in deployed products, do not put secrets in information cards or logs, and do not expose password placeholders unless there is an explicit local-installation requirement. The portal's JSON endpoints are the built-in UI's device-local protocol; they are not an authenticated remote-management API.

## Continue

- [Provisioning lifecycle](PROVISIONING_LIFECYCLE.md)
- [Portal UI and configuration](PORTAL_UI.md)
- [Portal content](PORTAL_CONTENT.md)
- [Integration recipes](recipes/README.md)

Back to [documentation](README.md) · [project overview](../README.md).
