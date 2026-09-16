#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager portal;
// WiFiManager reads this object while the portal is open, so it must outlive setup().
WiFiManagerParameter brokerHost("broker_host", "MQTT broker", "mqtt.local", 40);

void setup() {
    Serial.begin(115200);

    portal.portalAddParameter(&brokerHost);  // Adds an application-owned setting to the built-in form.

    // WiFiManager copies this read-only status section when it is registered.
    PortalInfoSection deviceInfo;
    deviceInfo.id = "device";
    deviceInfo.title = "Example device";
    deviceInfo.items = {
        {"firmware", "Firmware", "1.0.0"},
        {"sensor", "Sensor", "Ready"},
    };
    portal.portalAddInfoSection(deviceInfo);

    // This callout appears on the built-in portal overview.
    PortalHomeCard hint;
    hint.id = "hint";
    hint.title = "What this example adds";
    hint.kind = PortalHomeCardKind::Callout;
    hint.text = "A normal text setting, a status section, and a home-page callout.";
    portal.portalAddHomeCard(hint);

    portal.setSaveParamsCallback([](WiFiManager::WiFiManagerRequestArgs) {
        // Validate and persist a copy in the application; this example only reports it.
        Serial.print("MQTT broker selected: ");
        Serial.println(brokerHost.getValue());
    });
    portal.setConfigPortalTimeout(180);

    portal.autoConnect("WiFiManager Content", "example-pass");
}

void loop() {
    portal.process();  // Serves portal requests until provisioning completes or times out.
}
