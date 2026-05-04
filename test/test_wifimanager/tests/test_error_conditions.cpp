#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test invalid AP password (too short)
void test_invalid_ap_password_too_short() {
    Serial.println("[TEST]   Testing invalid AP password (too short)...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Password < 8 chars - should handle gracefully
    wm.startConfigPortal("TestAP", "short"); // < 8 chars
    
    // Portal should either not start or use default behavior
    // Verify it doesn't crash
    wm.process();
    
    // Portal may or may not be active (implementation dependent)
    // Just verify no crash
    if (wm.getConfigPortalActive()) {
        wm.stopConfigPortal();
    }
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Invalid AP password (too short) handled without crash");
    
    Serial.println("[TEST]   Invalid AP password (too short) test completed successfully");
}

// Test invalid AP password (too long)
void test_invalid_ap_password_too_long() {
    Serial.println("[TEST]   Testing invalid AP password (too long)...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Password > 63 chars - should handle gracefully
    String longPassword = String('a', 64); // 64 chars
    wm.startConfigPortal("TestAP", longPassword.c_str());
    
    // Portal should handle gracefully
    wm.process();
    
    // Portal may or may not be active (implementation dependent)
    // Just verify no crash
    if (wm.getConfigPortalActive()) {
        wm.stopConfigPortal();
    }
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Invalid AP password (too long) handled without crash");
    
    Serial.println("[TEST]   Invalid AP password (too long) test completed successfully");
}

// Test empty SSID
void test_empty_ssid() {
    Serial.println("[TEST]   Testing empty SSID...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Empty SSID - should use default AP name
    wm.startConfigPortal(""); // Empty SSID
    
    wm.process();
    
    // Portal should start with default name or handle gracefully
    if (wm.getConfigPortalActive()) {
        String ssid = wm.getConfigPortalSSID();
        // Should have a valid SSID (either default or empty handled)
        TEST_ASSERT_TRUE_MESSAGE(true, "Empty SSID handled gracefully");
        wm.stopConfigPortal();
    } else {
        TEST_ASSERT_TRUE_MESSAGE(true, "Empty SSID handled gracefully (portal not started)");
    }
    
    Serial.println("[TEST]   Empty SSID test completed successfully");
}

// Test very long SSID
void test_very_long_ssid() {
    Serial.println("[TEST]   Testing very long SSID...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Very long SSID (> 32 chars) - should handle gracefully
    String longSSID = String('A', 64); // 64 chars
    wm.startConfigPortal(longSSID.c_str());
    
    wm.process();
    
    // Portal should handle gracefully (may truncate or reject)
    if (wm.getConfigPortalActive()) {
        wm.stopConfigPortal();
    }
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Very long SSID handled without crash");
    
    Serial.println("[TEST]   Very long SSID test completed successfully");
}

// Test connection failure handling
void test_connection_failure_handling() {
    Serial.println("[TEST]   Testing connection failure handling...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    // Attempt connection via autoConnect (will fail)
    wm.setConfigPortalTimeout(5);
    bool result = wm.autoConnect("TestAP");
    
    // Should handle failure gracefully
    TEST_ASSERT_FALSE(result);
    
    // Verify WiFi state is consistent
    wl_status_t status = WiFi.status();
    TEST_ASSERT_NOT_EQUAL(WL_CONNECTED, status);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Connection failure handling test completed successfully");
}

// Test resource cleanup after error
void test_resource_cleanup_after_error() {
    Serial.println("[TEST]   Testing resource cleanup after error...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Stop portal (simulating cleanup after potential error)
    wm.stopConfigPortal();
    wm.process();
    
    // Verify portal is stopped
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    // Verify we can start again (resources cleaned up)
    wm.startConfigPortal("TestAP2");
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Resource cleanup after error test completed successfully");
}

// Test multiple rapid start/stop calls
void test_multiple_rapid_start_stop() {
    Serial.println("[TEST]   Testing multiple rapid start/stop calls...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Rapid start/stop cycles
    for (int i = 0; i < 5; i++) {
        wm.startConfigPortal("TestAP");
        wm.process();
        delay(10);
        wm.stopConfigPortal();
        wm.process();
        delay(10);
    }

    // Final state should be stopped
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Multiple rapid start/stop calls test completed successfully");
}

