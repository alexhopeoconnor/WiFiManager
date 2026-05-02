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

    Serial.println("[TEST]   WiFi meta JSON shape test completed successfully");
}

void test_api_info_json_shape() {
    Serial.println("[TEST]   Testing /api/info JSON shape...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);
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
    TEST_ASSERT_NOT_NULL(strstr(p, "\"showUpdate\":"));

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
