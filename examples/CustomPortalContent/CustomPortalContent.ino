#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager portal;
WiFiManagerParameter brokerHost("broker_host", "MQTT broker", "mqtt.local", 40);

void setup() {
    Serial.begin(115200);

    portal.portalAddParameter(&brokerHost);

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
        // A product validates and persists this value here; this demo only prints it.
        Serial.print("MQTT broker selected: ");
        Serial.println(brokerHost.getValue());
    });
    portal.setConfigPortalTimeout(180);

    portal.autoConnect("WiFiManager Content", "example-pass");
}

void loop() { portal.process(); }
