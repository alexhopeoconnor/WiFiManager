#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager portal;
WiFiManagerParameter brokerHost("broker_host", "MQTT broker", "mqtt.local", 40);

void setup() {
    Serial.begin(115200);

    portal.portalAddParameter(&brokerHost);
    portal.portalAddInfoSection({
        "device", "Example device",
        {{"firmware", "Firmware", "1.0.0"}, {"sensor", "Sensor", "Ready"}},
    });
    portal.portalAddHomeCard({
        "hint", "What this example adds", PortalHomeCardKind::Callout,
        "A normal text setting, a status section, and a home-page callout.", {},
    });

    portal.setSaveParamsCallback([](WiFiManager::WiFiManagerRequestArgs) {
        Serial.print("MQTT broker selected: ");
        Serial.println(brokerHost.getValue());
    });
    portal.setConfigPortalTimeout(180);

    portal.autoConnect("WiFiManager Content", "example-pass");
}

void loop() { portal.process(); }
