#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test configuration API methods
// Consolidated setter tests - verify setters don't crash and can be called multiple times
void test_configuration_setters() {
    Serial.println("[TEST]   Testing configuration setters...");
    
    WiFiManager wm;
    
    // Test timeout setters
    wm.setConfigPortalTimeout(0);
    wm.setConfigPortalTimeout(30);
    wm.setConfigPortalTimeout(300);
    wm.setConnectTimeout(0);
    wm.setConnectTimeout(20);
    wm.setConnectTimeout(60);
    
    // Test HTTP port
    wm.setHttpPort(80);
    wm.setHttpPort(8080);
    
    // Test WiFi quality and filtering
    wm.setMinimumSignalQuality(-1); // Disable filter
    wm.setMinimumSignalQuality(0);
    wm.setMinimumSignalQuality(50);
    wm.setMinimumSignalQuality(100);
    wm.setRemoveDuplicateAPs(true);
    wm.setRemoveDuplicateAPs(false);
    
    // Test display options
    wm.setShowStaticFields(true);
    wm.setShowStaticFields(false);
    wm.setShowDnsFields(true);
    wm.setShowDnsFields(false);
    
    // Test portal behavior
    wm.portalSetBehaviorCaptivePortalEnabled(true);
    wm.portalSetBehaviorCaptivePortalEnabled(false);
    
    // Test custom title path
    wm.portalSetBrandTitle("TestTitle");

    // Call setters multiple times with different values.
    wm.setConfigPortalTimeout(60);
    wm.setConnectTimeout(30);
    wm.setHttpPort(8080);
    wm.setMinimumSignalQuality(50);
    
    Serial.println("[TEST]   Configuration setters test completed successfully");
}

// Test hostname setter/getter - can verify behavior
void test_set_and_get_hostname() {
    Serial.println("[TEST]   Testing setHostname() and getWiFiHostname()...");
    
    WiFiManager wm;
    
    TEST_ASSERT_TRUE_MESSAGE(wm.setHostname("test-hostname"), "Valid char* hostname should be accepted");
    TEST_ASSERT_TRUE_MESSAGE(wm.setHostname(String("  trimmed-hostname  ")), "Valid String hostname should be trimmed and accepted");
    TEST_ASSERT_TRUE_MESSAGE(wm.setHostname(""), "Empty hostname should clear the configured hostname");

    String tooLongHostname = "123456789012345678901234567890123";
    TEST_ASSERT_FALSE_MESSAGE(wm.setHostname(tooLongHostname), "Hostnames longer than 32 chars should be rejected");
    TEST_ASSERT_FALSE_MESSAGE(wm.setHostname("bad host"), "Hostnames with spaces should be rejected");
    TEST_ASSERT_FALSE_MESSAGE(wm.setHostname("-leading"), "Hostnames cannot start with a hyphen");
    TEST_ASSERT_FALSE_MESSAGE(wm.setHostname("trailing-"), "Hostnames cannot end with a hyphen");

    (void)wm.getWiFiHostname();
    
    Serial.println("[TEST]   setHostname() and getWiFiHostname() test completed successfully");
}

// Test WiFi AP configuration setters - consolidated
void test_wifi_ap_configuration_setters() {
    Serial.println("[TEST]   Testing WiFi AP configuration setters...");
    
    WiFiManager wm;
    
    // Test AP channel
    wm.setWiFiAPChannel(0);  // Auto
    wm.setWiFiAPChannel(1);
    wm.setWiFiAPChannel(11);
    
    // Test AP hidden
    wm.setWiFiAPHidden(true);
    wm.setWiFiAPHidden(false);
    
    // Test country code
    wm.setCountry("US");
    wm.setCountry("CN");
    
    // Test clean connect
    wm.setCleanConnect(true);
    wm.setCleanConnect(false);
    
    // Test auto reconnect
    wm.setWiFiAutoReconnect(true);
    wm.setWiFiAutoReconnect(false);
    
    Serial.println("[TEST]   WiFi AP configuration setters test completed successfully");
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

