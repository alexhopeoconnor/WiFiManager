# Portal content and application settings

Use the built-in portal to collect small product settings alongside Wi-Fi credentials. WiFiManager owns the form and its temporary values; the application validates and persists its own settings.

This matches the product-firmware pattern used by real consumers: load configuration first, expose its current values through WiFiManagerParameter objects, then persist valid submitted values through the application's configuration layer.

## End-to-end pattern

~~~cpp
#include <WiFiManager.h>

constexpr int kBrokerHostLength = 64;

WiFiManager wifi;
WiFiManagerParameter brokerHost(
    "broker_host", "MQTT broker", "", kBrokerHostLength);

void setupPortal() {
    // The application has already loaded settings before this point.
    brokerHost.setValue(settings.mqttHost.c_str(), kBrokerHostLength);
    wifi.portalAddParameter(&brokerHost);

    wifi.setSaveParamsCallback([](WiFiManager::WiFiManagerRequestArgs) {
        const String candidate = brokerHost.getValue();

        if (!isValidHostname(candidate)) {
            logInvalidBroker(candidate);
            return;  // Keep the application's known-good stored value.
        }

        settings.mqttHost = candidate;
        saveApplicationSettings(settings);
    });
}
~~~

Call setupPortal() after loading application settings and before a WiFiManager connection or portal start method. The callback receives a copy of the submitted request arguments; use getArg(), getArgAsInt(), getArgAsFloat(), or getArgAsBool() when the application needs request-level values.

The save-parameters callback is a notification hook, not a validation-response API. Its return type cannot turn the built-in UI's success response into a form error. Validate before using the candidate, preserve known-good data when persistence fails, and provide product-specific feedback through the application's own UI/logging policy.

## Parameter rules

| Rule | Why it matters |
| --- | --- |
| Keep each WiFiManagerParameter alive while the portal can use it. | WiFiManager stores the parameter pointer; it does not take ownership. |
| Use a stable ID without spaces or special characters. | The ID is used in portal requests. |
| Choose a bounded value length. | It defines the editable buffer length. |
| Register parameters before opening the portal. | Active portal responses use their established form model. |
| Treat submitted text as untrusted application input. | Portal input is not application validation or durable storage. |

Use portalClearParameters() only before opening a portal when rebuilding a complete form. It removes registered parameter pointers; it does not destroy application-owned parameter objects.

## Where settings appear

By default, custom parameters appear on the Wi-Fi page. To put them on the separate Setup page:

~~~cpp
wifi.portalSetLayoutParamsLocation(PortalParamsLocation::SetupPage);
~~~

Use the Wi-Fi page for a small setting that is naturally provisioned with network credentials. Use the Setup page when product configuration needs its own step. Page visibility and layout are part of the structured portal policy; see [Portal UI and configuration](PORTAL_UI.md).

## Read-only product context

Use information sections for labelled facts and home cards for a short overview or callout:

~~~cpp
PortalInfoSection deviceInfo;
deviceInfo.id = "device";
deviceInfo.title = "Device";
deviceInfo.items = {
    {"firmware", "Firmware", firmwareVersion},
    {"sensor", "Sensor", sensorReady ? "Ready" : "Checking"},
};
wifi.portalAddInfoSection(deviceInfo);

PortalHomeCard installerHint;
installerHint.id = "installer-hint";
installerHint.title = "Before you begin";
installerHint.kind = PortalHomeCardKind::Callout;
installerHint.text = "Connect the device to its final local network.";
wifi.portalAddHomeCard(installerHint);
~~~

Information sections and cards are copied at registration. Do not place secrets, passwords, API tokens, or personally identifying values in them.

## Save callbacks

| Callback | Use |
| --- | --- |
| setPreSaveParamsCallback() | Observe a parameter-only save before the normal parameter callback. |
| setSaveParamsCallback(args) | Read, validate, and persist application settings after a parameter save. |
| setPreSaveConfigCallback() | Observe a combined Wi-Fi/config save before Wi-Fi connection processing. |
| setSaveConfigCallback() | React after Wi-Fi settings changed and the connection succeeded, or when break-after-config is enabled. |
| setConfigResetCallback() | Clear or reconcile application configuration when the portal resets Wi-Fi settings. |

Do not use a Wi-Fi-success callback to persist unrelated product settings: parameters can be saved separately, and an application needs its own durable-data policy.

The buildable [Custom Portal Content](../examples/CustomPortalContent/) example demonstrates parameters, information sections, and home cards. This guide supplies the missing persistence and validation boundary.

## Continue

- [Portal UI and configuration](PORTAL_UI.md)
- [Product settings and Wi-Fi recipe](recipes/PRODUCT_SETTINGS_AND_WIFI.md)
- [API reference](API_REFERENCE.md)

Back to [documentation](README.md) · [project overview](../README.md).
