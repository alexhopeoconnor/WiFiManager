# Integration recipes

These patterns come from real firmware that consumes this WiFiManager fork. They describe the boundary between a product application and WiFiManager; they are not additional framework APIs and do not require a cloud service or companion app.

| Scenario | Start here when… |
| --- | --- |
| [Field installer provisioning](FIELD_INSTALLER_PROVISIONING.md) | A physical device needs a deliberately bounded setup window on site. |
| [Product settings and Wi-Fi](PRODUCT_SETTINGS_AND_WIFI.md) | The portal collects Wi-Fi and application-owned settings together. |
| [Primary and fallback Wi-Fi](PRIMARY_AND_FALLBACK_WIFI.md) | A deployment supplies a candidate primary/fallback network that must be verified before storage. |
| [Local web-service handoff](LOCAL_WEB_SERVICE_HANDOFF.md) | The application already owns port 80 when recovery provisioning may begin. |
| [Provisioning state feedback](PROVISIONING_STATE_FEEDBACK.md) | LEDs, a display, or logs need to distinguish setup, recovery, and connected states. |

The normal [Basic Portal](../../examples/BasicPortal/) example remains the shortest way to try the legacy saved-network-or-portal flow. These recipes explain the product concerns that a standalone example should not pretend to solve.

Back to [documentation](../README.md) · [project overview](../../README.md).
