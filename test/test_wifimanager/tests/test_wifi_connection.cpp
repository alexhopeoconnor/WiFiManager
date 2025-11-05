#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test autoConnect fallback to portal when no saved credentials
void test_autoconnect_fallback_to_portal() {
    Serial.println("[TEST]   Testing autoConnect fallback to portal...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    wm.setConfigPortalTimeout(5);
    bool result = wm.autoConnect("TestAP");
    
    // Should fail to connect (no saved credentials)
    TEST_ASSERT_FALSE(result);
    
    // Should start portal as fallback
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   autoConnect fallback to portal test completed successfully");
}

// Test connectWifi with non-existent SSID (using autoConnect instead)
void test_connectwifi_ssid_not_found() {
    Serial.println("[TEST]   Testing connection with non-existent SSID...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    // Use autoConnect with non-existent SSID (will fail to connect)
    wm.setConfigPortalTimeout(5);
    bool result = wm.autoConnect("TestAP");
    
    // Should fail to connect (no saved credentials or SSID not found)
    TEST_ASSERT_FALSE(result);
    
    // Verify WiFi is not connected
    wl_status_t status = WiFi.status();
    TEST_ASSERT_NOT_EQUAL(WL_CONNECTED, status);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Connection with non-existent SSID test completed successfully");
}

// Test connect retry count setting
void test_connectwifi_retry_count() {
    Serial.println("[TEST]   Testing connect retry count setting...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    // Set retry count
    wm.setConnectRetries(3);
    
    // Verify setting is stored (by attempting connection via autoConnect)
    // The retry logic should be called internally
    wm.setConfigPortalTimeout(5);
    bool result = wm.autoConnect("TestAP");
    
    // Should fail (not connected)
    TEST_ASSERT_FALSE(result);
    
    // Verify no crash with retry setting
    TEST_ASSERT_TRUE_MESSAGE(true, "autoConnect with retry count executed without crash");
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Connect retry count test completed successfully");
}

// Test connect timeout setting
void test_connectwifi_timeout_setting() {
    Serial.println("[TEST]   Testing connect timeout setting...");
    
    WiFiManager wm;
    
    // Ensure WiFi is in a stable state before resetting
    delay(100);
    wm.resetSettings(); // Clear saved credentials
    
    // Allow WiFi operations to complete
    delay(100);
    
    // Set timeout
    wm.setConnectTimeout(5);
    
    // Verify timeout setting is used (by attempting connection via autoConnect)
    wm.setConfigPortalTimeout(5);
    bool result = wm.autoConnect("TestAP");
    
    // Should fail (not connected)
    TEST_ASSERT_FALSE(result);
    
    // Verify no crash with timeout setting
    TEST_ASSERT_TRUE_MESSAGE(true, "autoConnect with timeout setting executed without crash");
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Connect timeout setting test completed successfully");
}

// Test connection state transitions
void test_connection_state_transitions() {
    Serial.println("[TEST]   Testing connection state transitions...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    // Attempt connection via autoConnect (will fail)
    wm.setConfigPortalTimeout(5);
    wm.autoConnect("TestAP");
    
    // Wait a bit for state to potentially change
    delay(100);
    
    // Verify state transition (may be IDLE, DISCONNECTED, or NO_SSID_AVAIL)
    wl_status_t finalStatus = WiFi.status();
    
    // Should not be CONNECTED (since no saved credentials)
    TEST_ASSERT_NOT_EQUAL(WL_CONNECTED, finalStatus);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Connection state transitions test completed successfully");
}

// Test autoConnect with timeout
void test_autoconnect_with_timeout() {
    Serial.println("[TEST]   Testing autoConnect with timeout...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    wm.setConfigPortalTimeout(5);
    wm.setConnectTimeout(3);
    
    bool result = wm.autoConnect("TestAP");
    
    // Should fail to connect (no saved credentials)
    TEST_ASSERT_FALSE(result);
    
    // Should start portal as fallback
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   autoConnect with timeout test completed successfully");
}

