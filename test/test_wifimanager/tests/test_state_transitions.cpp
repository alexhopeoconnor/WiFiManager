#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test portal to connected transition (state consistency)
void test_portal_to_connected_transition() {
    Serial.println("[TEST]   Testing portal to connected transition...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Verify portal is active
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    TEST_ASSERT_FALSE(wm.getWebPortalActive());
    
    // Portal state should be consistent
    // While portal is active, WiFi should be in AP mode
    WiFiMode_t mode = WiFi.getMode();
    TEST_ASSERT_TRUE((mode & WIFI_AP) != 0);
    
    // Stop portal
    wm.stopConfigPortal();
    wm.process();
    
    // Verify portal is stopped
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Portal to connected transition test completed successfully");
}

// Test concurrent operations (portal + connection attempt)
void test_concurrent_operations() {
    Serial.println("[TEST]   Testing concurrent operations...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Attempt connection while portal is active (via preloadWiFi)
    // This should either be ignored or handled gracefully
    wm.preloadWiFi("NonExistentSSID_12345", "password");
    
    // Portal should still be active (connection attempt shouldn't stop it)
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Verify WiFi is not connected (SSID doesn't exist)
    wl_status_t status = WiFi.status();
    TEST_ASSERT_NOT_EQUAL(WL_CONNECTED, status);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Concurrent operations test completed successfully");
}

// Test state consistency during portal operation
void test_state_consistency_during_portal() {
    Serial.println("[TEST]   Testing state consistency during portal operation...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Verify state consistency
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    TEST_ASSERT_NOT_NULL(wm.server);
    TEST_ASSERT_NOT_NULL(wm.dnsServer);
    
    // Process multiple times
    for (int i = 0; i < 10; i++) {
        wm.process();
        delay(10);
        
        // State should remain consistent
        TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    }
    
    wm.stopConfigPortal();
    wm.process();
    
    // After stop, portal should be inactive
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   State consistency during portal operation test completed successfully");
}

// Test state transitions with multiple start/stop cycles
void test_state_transitions_multiple_cycles() {
    Serial.println("[TEST]   Testing state transitions with multiple cycles...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    for (int cycle = 0; cycle < 3; cycle++) {
        // Start state
        TEST_ASSERT_FALSE(wm.getConfigPortalActive());
        
        // Start portal
        wm.startConfigPortal("TestAP");
        wm.process();
        TEST_ASSERT_TRUE(wm.getConfigPortalActive());
        
        // Active state
        wm.process();
        delay(100);
        TEST_ASSERT_TRUE(wm.getConfigPortalActive());
        
        // Stop portal
        wm.stopConfigPortal();
        wm.process();
        TEST_ASSERT_FALSE(wm.getConfigPortalActive());
        
        delay(100);
    }
    
    Serial.println("[TEST]   State transitions with multiple cycles test completed successfully");
}

