# Product settings and Wi-Fi

A connected device often needs more than an SSID and password: a device name, broker host, endpoint, operating mode, or installer-selected option. The actual consuming framework pattern is to load those settings before WiFiManager begins, expose them as portal parameters, and persist them through the application's own storage layer.

## Flow

1. Load the application's durable settings and run its schema migration.
2. Build WiFiManagerParameter objects from the loaded values.
3. Register those parameters before a portal can start.
4. Register the save-parameters callback.
5. Validate and persist submitted application values through the application's storage layer.
6. Separately observe a successful Wi-Fi configuration/save if product services need a connected station first.

~~~cpp
void configurePortalFromSettings() {
    brokerHost.setValue(settings.mqttHost.c_str(), 64);
    wifi.portalAddParameter(&brokerHost);

    wifi.setSaveParamsCallback([](WiFiManager::WiFiManagerRequestArgs) {
        const String requestedHost = brokerHost.getValue();

        if (!isValidHostname(requestedHost)) {
            logRejectedSetting("broker_host");
            return;
        }

        settings.mqttHost = requestedHost;
        if (!saveApplicationSettings(settings)) {
            reportConfigurationStorageFailure();
        }
    });
}
~~~

WiFiManager does not own settings, schema migration, or the storage transaction. In particular, an application should keep known-good settings if validation or storage fails.

## Keep the two persistence paths separate

| Data | Owner | Save trigger |
| --- | --- | --- |
| Wi-Fi credentials in legacy mode | WiFiManager/platform Wi-Fi | Portal Wi-Fi save and connection flow |
| Primary/fallback station profiles | Application-provided profile store | Profile controller after verified candidate connection, or explicit save |
| Product settings | Application | setSaveParamsCallback() and application validation |
| Product schema/revision | Application | Application boot/migration policy |

A profile provisioning revision is not the same thing as an application schema version. The application owns both migration and the decision to apply a configuration update.

## Form placement

Use portalSetLayoutParamsLocation(PortalParamsLocation::WiFiPage) for one or two values that naturally belong in initial connectivity setup. Use SetupPage for product configuration that deserves a separate step. Put read-only status in PortalInfoSection or PortalHomeCard rather than turning it into a parameter.

For lifetime and callback details, see [Portal content](../PORTAL_CONTENT.md).
