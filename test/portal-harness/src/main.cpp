#include <Arduino.h>
#include <WiFiManager.h>

namespace {

#if defined(ESP8266)
constexpr char kPortalSsid[] = "WM Contract ESP8266";
#else
constexpr char kPortalSsid[] = "WM Contract ESP32";
#endif
constexpr char kPortalPassword[] = "default1";

WiFiManager wifi;
WiFiManagerParameter kInstallationLabel(
    "installation_label", "Installation label", "Contract fixture", 32);
// Keep the characters from upstream issue #1863 in the portal fixture. The
// browser contract verifies this value through JSON, DOM rendering, save, and
// a subsequent reload rather than relying on a string-only serializer check.
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
    "owner_name", "Owner name", "Portal contract", 48);
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

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(300);

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
    // Keep custom parameters on their own native Save parameters page. This
    // lets the browser contract exercise a parameter-only submit without
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
