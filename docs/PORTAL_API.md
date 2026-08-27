# Portal API

The portal uses JSON endpoints under `/api/...` for WiFi scans and saves, parameters, information, status, restart/erase/exit actions, captive-portal closure, and station-connect status. Consumers should treat the documented response shapes as the contract; portal HTML is not an API.

## Profile-mode WiFi metadata

When an application supplies a `WiFiManagerStationProfileStore`, `GET /api/wifi/meta` reports a primary profile and optional fallback without returning passwords. `POST /api/wifi/save` verifies a submitted candidate before committing it; see [station profiles](STATION_PROFILES.md) for fields and lifecycle.

## WiFi connect status

When `portalSetBehaviorConnectOnSave(true)` is enabled, saving credentials queues a station join. The SPA polls `GET /api/wifi/connect-status`:

```json
{
  "state": "waiting | success | failed",
  "message": "human readable status",
  "wifiStatus": "WL_CONNECTED",
  "stationIp": "192.168.1.42",
  "redirectUrl": "http://192.168.1.42/"
}
```

`stationIp` and `redirectUrl` are present only after a successful join. If the portal server is not on port 80, `redirectUrl` includes that port.

On success, WiFiManager keeps the portal alive briefly so the client can read the final status and navigate before the AP is shut down. Captive-portal helpers, DHCP timing, browser behaviour, and network isolation can still prevent an automatic redirect, so clients must handle a visible address as well.

## API design rules

- Use JSON endpoint results for state and actions.
- Keep UI-visible capabilities in the portal bootstrap/API payloads.
- Add a documented endpoint contract before adding a new portal workflow.
- Do not derive state by parsing the rendered shell.

Back to [documentation](README.md) · [project overview](../README.md).
