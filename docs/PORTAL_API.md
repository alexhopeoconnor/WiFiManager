# Built-in portal browser protocol

This device-local protocol is used by WiFiManager's built-in portal shell. It
is useful when maintaining that UI or writing portal-focused tests. It is not a
cloud API, remote-management interface, or supported companion-app integration
surface.

The portal is unauthenticated local setup infrastructure. Do not expose it as a product's general application API, and do not infer a security boundary from hiding an action in the UI.

## Contract version and format

GET /api/bootstrap returns contractVersion 3. The built-in portal uses that versioned response; tests that assert its shape should feature-detect fields rather than assume undocumented portal HTML or JSON properties.

All documented API responses are JSON with no-cache headers. The server also sends permissive CORS headers for the built-in portal implementation; that does not change the local-only security boundary or make these routes a remote integration contract.

## Route inventory

| Method | Route | Built-in portal purpose |
| --- | --- | --- |
| GET | / | Serves the one portal HTML shell. |
| GET | /api/bootstrap | Reads product brand, portal context, page/action visibility, layout, scan summary, and overview cards. |
| GET | /api/wifi/scan-status | Reads async scan state and visible nearby networks. |
| POST | /api/wifi/scan | Queues a forced asynchronous scan; returns 202. |
| GET | /api/wifi/meta | Reads Wi-Fi form fields, static-IP fields, parameters on the Wi-Fi page, and profile metadata when enabled. |
| POST | /api/wifi/save | Submits Wi-Fi fields, applicable parameters, and optionally a station-profile set. |
| GET | /api/wifi/connect-status | Reads the result of the most recent portal save-and-connect attempt. |
| POST | /api/wifi/connect-complete | Acknowledges that the built-in portal has received the successful station handoff address. |
| POST | /api/portal/timeout-reset | Restarts an active configured portal timeout. |
| GET | /api/params | Reads parameters for the separate Setup page. |
| POST | /api/params/save | Saves Setup-page parameters. |
| GET | /api/info | Reads device/Wi-Fi info, copied information sections, and visible actions. |
| GET | /api/status | Reads a short plain-text status summary in JSON. |
| POST | /api/device/restart | Schedules a target restart. |
| POST | /api/device/erase | Erases Wi-Fi configuration and schedules restart when successful. |
| POST | /api/portal/close | Disables captive-portal detection for the active session. |
| POST | /api/portal/exit | Requests portal exit when portal exit is allowed. |
| POST | /u | Receives multipart firmware upload and returns JSON completion status. |

Unknown routes are redirected only while captive-portal handling is active; otherwise they return the normal not-found result.

## Bootstrap and read models

Bootstrap includes a brand object (title, tagline, logo SVG, and logo alt text), a context object (portal activity, timeout remaining, identity text, status summary, scan state), visible pages/actions, parameter layout, and copied home cards.

Wi-Fi form metadata is intentionally separate:

- Legacy mode returns SSID/password fields, configured static fields, and parameters when the layout places them on the Wi-Fi page.
- Profile mode returns primary/fallback profile metadata, the active slot, controller state, static fields, and applicable parameters.
- Password values are never returned. Legacy password placeholder behavior is controlled by portalSetFieldPasswordPlaceholderMode().

GET /api/params exposes all registered parameters for the separate Setup page, plus whether the back action is visible. GET /api/info exposes device/Wi-Fi facts, copied information sections, and action visibility. GET /api/status returns a short human-readable text field.

## Wi-Fi submit and connection handoff

POST /api/wifi/save accepts form fields. In legacy mode:

| Field | Meaning |
| --- | --- |
| s | Station SSID. |
| p | Station password. A password without an SSID is treated as a password change for the stored SSID. |
| ip, gw, sn, dns | Optional station static IP, gateway, subnet, and DNS values when those fields are visible. |
| Registered parameter ID or param_N | Application parameter values when parameters are placed on the Wi-Fi page. |

A normal accepted save returns 202 and directs the portal to poll /api/wifi/connect-status. The status response is:

~~~json
{
  "state": "success",
  "message": "human readable status",
  "wifiStatus": "WL_CONNECTED",
  "stationIp": "192.168.1.42",
  "redirectUrl": "http://192.168.1.42/"
}
~~~

`state` is one of `idle`, `waiting`, `success`, or `failed`. The example shows
a successful join; `stationIp` and `redirectUrl` are present only in that state.
If the portal server is not on port 80, `redirectUrl` includes that port. When
connect-on-save is disabled, a saved configuration has no station address and
the built-in portal remains open.

After observing success, the built-in portal POSTs /api/wifi/connect-complete. A 409 response means successful handoff is not ready; otherwise WiFiManager keeps the portal alive briefly, receives the acknowledgement, and then closes after a grace delay. Browser captive redirects can still fail, so the portal keeps the station address visible.

When portalSetBehaviorConnectOnSave(false) or setSaveConnect(false) is selected, a Wi-Fi save does not start this station connection/handoff flow.

## Profile-mode submit

In profile mode, POST /api/wifi/save accepts s0/p0 for primary and s1/p1 for fallback. Primary must be non-empty. A blank p0 or p1 preserves an existing password; clear0 or clear1 explicitly clears a password for an open network.

By default, WiFiManager attempts the submitted candidate and returns 202 for status polling. With stationAction=save, it writes the submitted profile set for a later connection attempt and returns 200. See [Station profiles](STATION_PROFILES.md) for candidate verification and storage behavior.

## Parameter submit

POST /api/params/save sends registered application parameter fields and returns a successful acknowledgement after the registered save callbacks run. A parameter can be addressed by its stable ID or by param_N index. WiFiManager does not define application validation or durable-storage semantics; the consuming application owns both.

## Actions and error states

| Route | Success | Important non-success response |
| --- | --- | --- |
| POST /api/wifi/scan | 202 with accepted/queued state | Scan result arrives through scan-status. |
| POST /api/wifi/save | 202 for queued connection, or 200 for profile save-for-later | 400 for invalid Wi-Fi/profile input; 500 when an explicit profile store save fails. |
| POST /api/wifi/connect-complete | 200 after successful station handoff | 409 when success/address is not ready. |
| POST /api/portal/timeout-reset | 200 with timeout seconds remaining | 409 when no active finite portal timeout exists. |
| POST /api/device/restart | 200, restart scheduled | The target restarts shortly after the response. |
| POST /api/device/erase | 200, erase/restart scheduled | 500 when erase fails. |
| POST /api/portal/close | 200, captive detection disabled | The portal server itself remains subject to its normal lifecycle. |
| POST /api/portal/exit | 200, exit scheduled | 403 when exit is not allowed. |
| POST /u | 200, firmware update/restart scheduled | 500 with update failure detail. |

## Using this protocol safely

Use this document to understand and test WiFiManager's own portal behavior.
Applications that need a product web API should host and secure that API after
WiFiManager has completed its provisioning role. Do not scrape the portal shell,
depend on undocumented JSON fields, or add routes through WiFiManager's testing
server accessor.

Back to [documentation](README.md) · [project overview](../README.md).
