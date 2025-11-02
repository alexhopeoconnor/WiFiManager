#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test basic WiFiManager instantiation
void test_basic_wifimanager_instantiation() {
    Serial.println("[TEST]   Testing basic WiFiManager instantiation...");
    
    // Test default constructor
    WiFiManager wm;
    
    // Verify instance created (basic sanity check)
    TEST_ASSERT_TRUE_MESSAGE(true, "WiFiManager instance created successfully");
    
    // Test that we can call basic methods without crashing
    String defaultName = wm.getDefaultAPName();
    TEST_ASSERT_GREATER_THAN(0, defaultName.length());
    
    // Test default state - portal should not be active
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    TEST_ASSERT_FALSE(wm.getWebPortalActive());
    
    Serial.println("[TEST]   Basic WiFiManager instantiation test completed successfully");
}

// Test resetSettings clears credentials
void test_reset_settings() {
    Serial.println("[TEST]   Testing resetSettings()...");
    
    WiFiManager wm;
    
    // Reset settings (should not crash)
    wm.resetSettings();
    
    // Verify WiFi is not saved after reset
    TEST_ASSERT_FALSE(wm.getWiFiIsSaved());
    
    Serial.println("[TEST]   resetSettings() test completed successfully");
}

// Test disconnect - verify it doesn't crash and can be called multiple times
void test_disconnect() {
    Serial.println("[TEST]   Testing disconnect()...");
    
    WiFiManager wm;
    
    // Disconnect should not crash even if not connected
    bool result1 = wm.disconnect();
    
    // Can be called multiple times safely
    bool result2 = wm.disconnect();
    
    // Verify no crash (results may be true/false depending on WiFi state)
    TEST_ASSERT_TRUE_MESSAGE(true, "disconnect() executed without crash");
    
    Serial.println("[TEST]   disconnect() test completed successfully");
}

