# Portal customisation

The stable extension surface is the `portal*` API and JSON API contract. Templates, undocumented DOM IDs, and SPA routing internals are not consumer extension points.

## Supported changes

- `portalSetBrand*` and `portalSetContextIdentityText()` for the visible identity.
- `portalSetPage*`, `portalSetAction*`, `portalSetLayout*`, and `portalSetBehavior*` for built-in capability and layout choices.
- `portalAddParameter()`, including raw HTML blocks inside parameter-rendering surfaces.
- `portalAddInfoSection()` and `portalAddHomeCard()` for structured content.
- `portalAppendCss()`, `portalOverrideCss()`, and `portalAppendJs()` for small presentation enhancements.

```cpp
WiFiManager wm;

wm.portalSetBrandTitle("Solar Battery Monitor Setup");
wm.portalSetContextIdentityText("Solar Battery Monitor");
wm.portalSetBrandHomeIntro(
    "Connect the monitor to WiFi, then review its battery settings."
);
wm.portalSetPageUpdateVisible(false);
wm.portalSetActionEraseVisible(false);
wm.portalSetLayoutParamsLocation(PortalParamsLocation::SetupPage);

WiFiManagerParameter mqttHost(
    "mqtt_host", "MQTT host", "broker.local", 64,
    "placeholder='broker.local'"
);
wm.portalAddParameter(&mqttHost);

PortalHomeCard summary;
summary.id = "battery";
summary.title = "Battery";
summary.kind = PortalHomeCardKind::KeyValue;
summary.items.push_back({"voltage", "Voltage", "13.2V"});
wm.portalAddHomeCard(summary);

wm.portalAppendJs(
    "document.addEventListener('wm:ready', function(event) {"
    "  console.log('Portal ready', event.detail.boot);"
    "});"
);
```

## Enhancement events

Appended JavaScript may enhance the existing SPA through:

- `wm:ready`, with `event.detail.boot`;
- `wm:view-changed`, with `event.detail.route`.

It should not replace routing or built-in action flow.

## Intentional boundary

The following are fork-level changes, not supported consumer customisation:

- arbitrary home, information, navigation, or shell HTML injection;
- replacing built-in SPA routes or action flows;
- depending on undocumented DOM IDs or private templates;
- adding a new backend-to-frontend workflow without an API contract.

For data and endpoint behaviour, see [Portal API](PORTAL_API.md).

Back to [documentation](README.md) · [project overview](../README.md).
