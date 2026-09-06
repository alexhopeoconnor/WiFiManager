# Local web-service handoff

A product may already host its own local HTTP service when a later connection failure opens WiFiManager's provisioning portal. Because WiFiManager owns its portal server port while active, the product must release that port before WiFiManager registers its portal routes.

A real consuming framework does this from the AP callback: it shuts down its normal web service after the setup AP has started but before WiFiManager registers portal routes.

## Automatic recovery handoff

~~~cpp
void setup() {
    wifi.setAPCallback([](WiFiManager*) {
        stopApplicationWebServer();  // Releases port 80 for WiFiManager.
        setIndicator(IndicatorState::Setup);
    });

    wifi.autoConnect("Device Setup", "setup-password");
}
~~~

The AP callback runs after AP mode begins and before the portal routes are registered. It is the appropriate hook for automatic portal fallback. If the application explicitly starts setup itself, release its server before calling startConfigPortal().

## After a successful connection

The portal's successful Wi-Fi connection means the station has an address. It does not guarantee that the product's own web application, MQTT connection, or sensors are ready. The product should:

1. observe didConfigPortalConnectSucceed() or the matching event;
2. persist any application settings required for normal operation;
3. choose whether to restart or recreate its own server;
4. only advertise the product service when it is ready.

If the product uses a non-default WiFiManager HTTP port, it can avoid a port conflict but must treat the portal URL and its station-connect handoff URL as that non-default port.

Do not add routes to WiFiManager's internal server through getServer(). That accessor is for testing/host integration, not a supported product-extension API.

Continue with [Provisioning lifecycle](../PROVISIONING_LIFECYCLE.md) and [Observability](../OBSERVABILITY.md).
