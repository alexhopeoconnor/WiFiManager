#include <Arduino.h>
#include <WiFiManager.h>

namespace {

// These markers belong only to the portal hardware fixture. They are compiled
// into the image rather than stored in Wi-FiManager settings, so an A -> B
// assertion proves that the new firmware booted after the updater restarted
// the board. Normal portal test-harness builds retain a descriptive fixture
// value.
#if defined(WM_OTA_TEST_IMAGE)
// PlatformIO releases the serial port only after the upload-triggered reset.
// The physical OTA test harness then attaches passively, so give it the same
// explicit window as the DeviceFramework A/B fixtures before boot evidence or
// portal work begins. Normal portal test-harness builds keep the short delay.
constexpr unsigned long kSerialMonitorAttachDelayMs = 5000UL;
#else
#define WM_OTA_TEST_IMAGE "portal-harness"
constexpr unsigned long kSerialMonitorAttachDelayMs = 300UL;
#endif

#if defined(ESP8266)
constexpr char kPortalSsid[] = "WM Test Harness ESP8266";
#else
constexpr char kPortalSsid[] = "WM Test Harness ESP32";
#endif
constexpr char kPortalPassword[] = "default1";

WiFiManager wifi;
WiFiManagerParameter kInstallationLabel(
    "installation_label", "Installation label", "Harness fixture", 32);
// Keep the characters from upstream issue #1863 in the portal fixture. The
// browser test harness verifies this value through JSON, DOM rendering, save,
// and a subsequent reload rather than relying on a string-only serializer
// check.
WiFiManagerParameter kEscapedValue(
    "escaped_value", "Escaped value", "7(f+4]2y3fsYTQt'Uhxc\"d\\<>&", 64);
WiFiManagerParameter kMqttHost(
    "mqtt_host", "MQTT host", "broker.example.local", 64);
WiFiManagerParameter kMqttPort(
    "mqtt_port", "MQTT port", "1883", 8);
WiFiManagerParameter kDeviceRoom(
    "device_room", "Device room", "Workshop", 32);
WiFiManagerParameter kSensorName(
    "sensor_name", "Sensor name", "Ambient temperature", 48);
WiFiManagerParameter kTelemetryTopic(
    "telemetry_topic", "Telemetry topic", "sensors/ambient/temperature", 64);
WiFiManagerParameter kTimezone(
    "timezone", "Timezone", "Australia/Brisbane", 48);
WiFiManagerParameter kLatitude(
    "latitude", "Latitude", "-27.4698", 16);
WiFiManagerParameter kLongitude(
    "longitude", "Longitude", "153.0251", 16);
WiFiManagerParameter kFirmwareChannel(
    "firmware_channel", "Firmware channel", "stable", 16);
WiFiManagerParameter kOwnerName(
    "owner_name", "Owner name", "Portal test harness", 48);
WiFiManagerParameter kNotes(
    "notes", "Notes", "Thirteen-field rendering fixture", 64);

WiFiManagerParameter* const kPortalParameters[] = {
    &kInstallationLabel,
    &kEscapedValue,
    &kMqttHost,
    &kMqttPort,
    &kDeviceRoom,
    &kSensorName,
    &kTelemetryTopic,
    &kTimezone,
    &kLatitude,
    &kLongitude,
    &kFirmwareChannel,
    &kOwnerName,
    &kNotes,
};

void registerOtaTestMarker() {
    // setWebServerCallback runs after WiFiManager creates its server and
    // before it registers built-in routes. This private fixture endpoint is
    // intentionally not a WiFiManager product API.
    wifi.setWebServerCallback([]() {
        AsyncWebServer* const server = wifi.getServer();
        if (server == nullptr) {
            return;
        }

        server->on("/api/test/firmware-marker", HTTP_GET,
                   [](AsyncWebServerRequest* request) {
                       String response = F("{\"marker\":\"");
                       response += WM_OTA_TEST_IMAGE;
                       response += F("\",\"freeSketchSpace\":");
                       response += String(ESP.getFreeSketchSpace());
                       response += F("}");
                       request->send(200, "application/json", response);
                   });
    });
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(kSerialMonitorAttachDelayMs);
    // The physical HTTP OTA test harness records this immutable marker before
    // and after its browser upload. It cannot be faked by saved portal values
    // or an HTTP response from a stale image.
    Serial.print(F("WiFiManager portal OTA fixture image: "));
    Serial.println(WM_OTA_TEST_IMAGE);

    // This fixture must be independent of whichever sketch was previously
    // flashed to the board. Clear saved station credentials before starting
    // the portal so browser artifacts always show the unconfigured flow.
    wifi.resetSettings();

    // A finite window exercises timeout reset without leaving a board in a
    // permanent portal.
    wifi.setConfigPortalTimeout(15 * 60);
    wifi.setAPStaticIPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0));
    registerOtaTestMarker();
    // Keep custom parameters on their own native Save parameters page. This
    // lets the browser test harness exercise a parameter-only submit without
    // starting a station connection as part of the regression test.
    wifi.portalSetLayoutParamsLocation(PortalParamsLocation::SetupPage);
    for (auto* parameter : kPortalParameters) {
        wifi.portalAddParameter(parameter);
    }
    wifi.startConfigPortal(kPortalSsid, kPortalPassword);
}

void loop() {
    wifi.process();
}
