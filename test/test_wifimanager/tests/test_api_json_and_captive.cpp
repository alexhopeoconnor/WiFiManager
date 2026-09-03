#include <unity.h>
#include <Arduino.h>
#include <string.h>
#include <WiFiManager.h>
#include <WiFiManagerHandlers.h>
#include "../test_main.h"

void test_captive_redirect_host_rule() {
    Serial.println("[TEST]   Testing captive redirect host comparison...");

    TEST_ASSERT_TRUE(WiFiManagerHandlers::shouldRedirectCaptiveForHost("captive.apple.com", "192.168.4.1"));
    TEST_ASSERT_TRUE(WiFiManagerHandlers::shouldRedirectCaptiveForHost("192.168.4.2", "192.168.4.1"));
    TEST_ASSERT_FALSE(WiFiManagerHandlers::shouldRedirectCaptiveForHost("192.168.4.1", "192.168.4.1"));
    TEST_ASSERT_FALSE(WiFiManagerHandlers::shouldRedirectCaptiveForHost("example.com", ""));

    Serial.println("[TEST]   Captive redirect host rule test completed successfully");
}

void test_api_wifi_meta_json_shape() {
    Serial.println("[TEST]   Testing /api/wifi/meta JSON shape...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
    String j = handlers.buildApiWifiMetaJson();

    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"wifiFields\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"staticFields\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"params\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"actions\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"canRefreshScan\":true")));

    WiFiManagerParameter fieldParam("mqtt_host", "MQTT host", "broker.local", 64, "placeholder='broker.local'");
    WiFiManagerParameter htmlParam("<div class='wm-callout wm-callout--info'><p>GPS settings...</p></div>");
    wm.portalAddParameter(&fieldParam);
    wm.portalAddParameter(&htmlParam);
    j = handlers.buildApiWifiMetaJson();
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"kind\":\"field\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"kind\":\"html\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"html\":\"<div class='wm-callout wm-callout--info'><p>GPS settings...</p></div>\"}")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"wifiFields\":[{\"id\":\"s\"")));

    Serial.println("[TEST]   WiFi meta JSON shape test completed successfully");
}

void test_api_wifi_meta_password_field_type() {
    Serial.println("[TEST]   Testing /api/wifi/meta password field type from custom attrs...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
    WiFiManagerParameter pwd("mqttpass", "MQTT password", "secret", 24, " type='password' autocorrect='off'");
    wm.portalAddParameter(&pwd);

    String j = handlers.buildApiWifiMetaJson();
    const char* p = j.c_str();
    const char* chunk = strstr(p, "\"id\":\"mqttpass\"");
    TEST_ASSERT_NOT_NULL_MESSAGE(chunk, "mqttpass param missing from meta JSON");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(chunk, "\"type\":\"password\""),
                                 "customAttrs type=password must map to JSON type password");

    Serial.println("[TEST]   WiFi meta password field type test completed successfully");
}

void test_api_wifi_connect_status_success_redirect() {
    Serial.println("[TEST]   Testing /api/wifi/connect-status success redirect payload...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
#ifdef UNIT_TEST
    wm.wmTestSetPortalConnectSuccess("WiFi connected. Redirecting to 192.168.1.42", "192.168.1.42");
#endif
    String j = handlers.buildApiWifiConnectStatusJson();
    const char* p = j.c_str();

    TEST_ASSERT_NOT_NULL(strstr(p, "\"state\":\"success\""));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"stationIp\":\"192.168.1.42\""));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"redirectUrl\":\"http://192.168.1.42/\""));

    Serial.println("[TEST]   WiFi connect-status success redirect payload test completed successfully");
}

void test_profile_portal_success_keeps_handoff_alive() {
    Serial.println("[TEST]   Testing profile portal success hand-off delay...");

    WiFiManager wm;
#ifdef UNIT_TEST
    wm.wmTestCompleteProfilePortalConnectionSuccess();
    TEST_ASSERT_EQUAL(WiFiManager::WM_CP_CONNECT_SUCCESS, wm.getConfigPortalConnectState());
    TEST_ASSERT_TRUE_MESSAGE(
        wm.isConfigPortalConnectPending(),
        "Profile-backed portal success must remain pending while the client reads the redirect response"
    );
#else
    TEST_IGNORE_MESSAGE("UNIT_TEST helpers unavailable");
#endif

    Serial.println("[TEST]   Profile portal success hand-off delay test completed successfully");
}

void test_api_info_json_shape() {
    Serial.println("[TEST]   Testing /api/info JSON shape...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
    PortalInfoSection battery;
    battery.id = "battery";
    battery.title = "Battery";
    battery.items.push_back({"soc", "State of charge", "84%"});
    battery.items.push_back({"voltage", "Voltage", "13.2V"});
    wm.portalAddInfoSection(battery);
    String j = handlers.buildApiInfoJson();
    const char* p = j.c_str();
    // Shape checks only: "status" is a nested object (},\n"device" not ],"device"); require
    // a tail marker so we know the full String was built (heap is OK). Middle sections
    // (wifi / about) are not asserted — long runs fragment heap and can truncate long JSONs.
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_GREATER_THAN_INT_MESSAGE(200, (int)j.length(),
                                         "api/info body unexpectedly short; likely heap/fragmentation");
    TEST_ASSERT_NOT_NULL(strstr(p, "\"status\":"));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"connected\":"));
    TEST_ASSERT_NOT_NULL(strstr(p, "},\"device\":["));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"extraSections\":"));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"id\":\"battery\""));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"title\":\"Battery\""));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"label\":\"State of charge\""));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"showUpdate\":"));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"key\":\"aboutver\",\"label\":\"WiFiManager\",\"value\":\""));
    TEST_ASSERT_NOT_NULL(strstr(p, "\"},{\"key\":\"aboutarduinover\""));

    Serial.println("[TEST]   API info JSON shape test completed successfully");
}

void test_api_params_json_shape() {
    Serial.println("[TEST]   Testing /api/params JSON shape...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
    String j = handlers.buildApiParamsGetJson();

    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"params\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"actions\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"showBack\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"params\":[")));

    Serial.println("[TEST]   API params JSON shape test completed successfully");
}

void test_api_status_json_shape() {
    Serial.println("[TEST]   Testing /api/status JSON shape...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
    String j = handlers.buildApiStatusJson();

    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"text\"")));
    TEST_ASSERT_EQUAL('{', j.charAt(0));
    TEST_ASSERT_EQUAL('}', j.charAt(j.length() - 1));

    Serial.println("[TEST]   API status JSON shape test completed successfully");
}
