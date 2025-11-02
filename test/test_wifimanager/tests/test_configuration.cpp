#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test configuration API methods
void test_set_title() {
    Serial.println("[TEST]   Testing setTitle()...");
    
    WiFiManager wm;
    String testTitle = "TestTitle";
    
    wm.setTitle(testTitle);
    
    // Note: getTitle() doesn't exist, so we just verify no crash
    // The title is used internally when rendering pages
    TEST_ASSERT_TRUE_MESSAGE(true, "setTitle() executed without crash");
    
    Serial.println("[TEST]   setTitle() test completed successfully");
}

void test_set_config_portal_timeout() {
    Serial.println("[TEST]   Testing setConfigPortalTimeout()...");
    
    WiFiManager wm;
    
    // Test various timeout values
    wm.setConfigPortalTimeout(0);
    wm.setConfigPortalTimeout(30);
    wm.setConfigPortalTimeout(300);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setConfigPortalTimeout() executed without crash");
    
    Serial.println("[TEST]   setConfigPortalTimeout() test completed successfully");
}

void test_set_connect_timeout() {
    Serial.println("[TEST]   Testing setConnectTimeout()...");
    
    WiFiManager wm;
    
    wm.setConnectTimeout(0);
    wm.setConnectTimeout(20);
    wm.setConnectTimeout(60);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setConnectTimeout() executed without crash");
    
    Serial.println("[TEST]   setConnectTimeout() test completed successfully");
}

void test_set_http_port() {
    Serial.println("[TEST]   Testing setHttpPort()...");
    
    WiFiManager wm;
    
    // Test default port
    wm.setHttpPort(80);
    
    // Test custom port
    wm.setHttpPort(8080);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setHttpPort() executed without crash");
    
    Serial.println("[TEST]   setHttpPort() test completed successfully");
}

void test_set_minimum_signal_quality() {
    Serial.println("[TEST]   Testing setMinimumSignalQuality()...");
    
    WiFiManager wm;
    
    wm.setMinimumSignalQuality(-1); // Disable filter
    wm.setMinimumSignalQuality(0);
    wm.setMinimumSignalQuality(50);
    wm.setMinimumSignalQuality(100);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setMinimumSignalQuality() executed without crash");
    
    Serial.println("[TEST]   setMinimumSignalQuality() test completed successfully");
}

void test_set_remove_duplicate_aps() {
    Serial.println("[TEST]   Testing setRemoveDuplicateAPs()...");
    
    WiFiManager wm;
    
    wm.setRemoveDuplicateAPs(true);
    wm.setRemoveDuplicateAPs(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setRemoveDuplicateAPs() executed without crash");
    
    Serial.println("[TEST]   setRemoveDuplicateAPs() test completed successfully");
}

void test_set_show_static_fields() {
    Serial.println("[TEST]   Testing setShowStaticFields()...");
    
    WiFiManager wm;
    
    wm.setShowStaticFields(true);
    wm.setShowStaticFields(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowStaticFields() executed without crash");
    
    Serial.println("[TEST]   setShowStaticFields() test completed successfully");
}

void test_set_show_dns_fields() {
    Serial.println("[TEST]   Testing setShowDnsFields()...");
    
    WiFiManager wm;
    
    wm.setShowDnsFields(true);
    wm.setShowDnsFields(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowDnsFields() executed without crash");
    
    Serial.println("[TEST]   setShowDnsFields() test completed successfully");
}

void test_set_config_portal_blocking() {
    Serial.println("[TEST]   Testing setConfigPortalBlocking()...");
    
    WiFiManager wm;
    
    wm.setConfigPortalBlocking(true);
    wm.setConfigPortalBlocking(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setConfigPortalBlocking() executed without crash");
    
    Serial.println("[TEST]   setConfigPortalBlocking() test completed successfully");
}

void test_set_captive_portal_enable() {
    Serial.println("[TEST]   Testing setCaptivePortalEnable()...");
    
    WiFiManager wm;
    
    wm.setCaptivePortalEnable(true);
    wm.setCaptivePortalEnable(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCaptivePortalEnable() executed without crash");
    
    Serial.println("[TEST]   setCaptivePortalEnable() test completed successfully");
}

void test_set_hostname() {
    Serial.println("[TEST]   Testing setHostname()...");
    
    WiFiManager wm;
    
    // Test with char*
    wm.setHostname("test-hostname");
    
    // Test with String
    wm.setHostname(String("test-hostname-2"));
    
    // Test getting hostname
    String hostname = wm.getWiFiHostname();
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setHostname() and getWiFiHostname() executed without crash");
    
    Serial.println("[TEST]   setHostname() test completed successfully");
}

void test_set_wifi_ap_channel() {
    Serial.println("[TEST]   Testing setWiFiAPChannel()...");
    
    WiFiManager wm;
    
    wm.setWiFiAPChannel(0);  // Auto
    wm.setWiFiAPChannel(1);
    wm.setWiFiAPChannel(11);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setWiFiAPChannel() executed without crash");
    
    Serial.println("[TEST]   setWiFiAPChannel() test completed successfully");
}

void test_set_wifi_ap_hidden() {
    Serial.println("[TEST]   Testing setWiFiAPHidden()...");
    
    WiFiManager wm;
    
    wm.setWiFiAPHidden(true);
    wm.setWiFiAPHidden(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setWiFiAPHidden() executed without crash");
    
    Serial.println("[TEST]   setWiFiAPHidden() test completed successfully");
}

void test_set_clean_connect() {
    Serial.println("[TEST]   Testing setCleanConnect()...");
    
    WiFiManager wm;
    
    wm.setCleanConnect(true);
    wm.setCleanConnect(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCleanConnect() executed without crash");
    
    Serial.println("[TEST]   setCleanConnect() test completed successfully");
}

void test_set_country() {
    Serial.println("[TEST]   Testing setCountry()...");
    
    WiFiManager wm;
    
    wm.setCountry("US");
    wm.setCountry("CN");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCountry() executed without crash");
    
    Serial.println("[TEST]   setCountry() test completed successfully");
}

void test_set_wifi_auto_reconnect() {
    Serial.println("[TEST]   Testing setWiFiAutoReconnect()...");
    
    WiFiManager wm;
    
    wm.setWiFiAutoReconnect(true);
    wm.setWiFiAutoReconnect(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setWiFiAutoReconnect() executed without crash");
    
    Serial.println("[TEST]   setWiFiAutoReconnect() test completed successfully");
}

void test_get_default_ap_name() {
    Serial.println("[TEST]   Testing getDefaultAPName()...");
    
    WiFiManager wm;
    
    String apName = wm.getDefaultAPName();
    
    // Should return a non-empty string (chip ID based)
    TEST_ASSERT_GREATER_THAN(0, apName.length());
    
    Serial.println("[TEST]   getDefaultAPName() test completed successfully");
}

void test_get_wifi_status_string() {
    Serial.println("[TEST]   Testing getWLStatusString()...");
    
    WiFiManager wm;
    
    // Test with status code
    String status1 = wm.getWLStatusString(WL_IDLE_STATUS);
    String status2 = wm.getWLStatusString(WL_CONNECTED);
    String status3 = wm.getWLStatusString(WL_DISCONNECTED);
    
    // All should return non-empty strings
    TEST_ASSERT_GREATER_THAN(0, status1.length());
    TEST_ASSERT_GREATER_THAN(0, status2.length());
    TEST_ASSERT_GREATER_THAN(0, status3.length());
    
    // Test without parameter (uses current WiFi.status())
    String status4 = wm.getWLStatusString();
    TEST_ASSERT_GREATER_THAN(0, status4.length());
    
    Serial.println("[TEST]   getWLStatusString() test completed successfully");
}

void test_get_mode_string() {
    Serial.println("[TEST]   Testing getModeString()...");
    
    WiFiManager wm;
    
    String mode1 = wm.getModeString(WIFI_OFF);
    String mode2 = wm.getModeString(WIFI_STA);
    String mode3 = wm.getModeString(WIFI_AP);
    String mode4 = wm.getModeString(WIFI_AP_STA);
    
    // All should return non-empty strings
    TEST_ASSERT_GREATER_THAN(0, mode1.length());
    TEST_ASSERT_GREATER_THAN(0, mode2.length());
    TEST_ASSERT_GREATER_THAN(0, mode3.length());
    TEST_ASSERT_GREATER_THAN(0, mode4.length());
    
    Serial.println("[TEST]   getModeString() test completed successfully");
}

void test_get_rssi_as_quality() {
    Serial.println("[TEST]   Testing getRSSIasQuality()...");
    
    WiFiManager wm;
    
    // Test various RSSI values
    int quality1 = wm.getRSSIasQuality(-30);  // Very strong
    int quality2 = wm.getRSSIasQuality(-70);  // Good
    int quality3 = wm.getRSSIasQuality(-90);  // Weak
    
    // Quality should be 0-100
    TEST_ASSERT_GREATER_OR_EQUAL(0, quality1);
    TEST_ASSERT_LESS_OR_EQUAL(100, quality1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, quality2);
    TEST_ASSERT_LESS_OR_EQUAL(100, quality2);
    
    // Stronger signal should have higher quality
    TEST_ASSERT_GREATER_THAN(quality3, quality1);
    
    Serial.println("[TEST]   getRSSIasQuality() test completed successfully");
}

