#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test autoConnect fallback flow
void test_autoconnect_fallback_flow() {
    Serial.println("[TEST]   Testing autoConnect fallback flow...");
    
    WiFiManager wm;
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings(); // Clear saved credentials
    delay(100); // Allow WiFi operations to complete
    
    wm.setConfigPortalTimeout(5);
    
    // 1. Device starts
    // 2. autoConnect() called with no saved credentials
    bool result = wm.autoConnect("TestAP");
    
    // 3. Should fail to connect (no saved credentials)
    TEST_ASSERT_FALSE(result);
    
    // 4. Portal should start automatically as fallback
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   autoConnect fallback flow test completed successfully");
}

// Test portal lifecycle with connection attempt
void test_portal_lifecycle_with_connection_attempt() {
    Serial.println("[TEST]   Testing portal lifecycle with connection attempt...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // 1. Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // 2. Attempt connection (while portal active) via preloadWiFi
    wm.preloadWiFi("NonExistentSSID_12345", "password");
    
    // 3. Verify state handling - portal should still be active
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Connection should fail (verify WiFi is not connected)
    wl_status_t status = WiFi.status();
    TEST_ASSERT_NOT_EQUAL(WL_CONNECTED, status);
    
    // 4. Stop portal
    wm.stopConfigPortal();
    wm.process();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Portal lifecycle with connection attempt test completed successfully");
}

// Test parameter add and retrieve in portal
void test_parameter_add_and_retrieve() {
    Serial.println("[TEST]   Testing parameter add and retrieve in portal...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // 1. Add parameters
    WiFiManagerParameter param("server", "Server", "default", 40);
    wm.addParameter(&param);
    
    TEST_ASSERT_EQUAL(1, wm.getParametersCount());
    
    // 2. Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // 3. Verify parameters are accessible
    TEST_ASSERT_EQUAL(1, wm.getParametersCount());
    
    WiFiManagerParameter** params = wm.getParameters();
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_STRING("server", params[0]->getID());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Parameter add and retrieve in portal test completed successfully");
}

// Test complete flow: reset -> autoConnect -> portal
void test_complete_flow_reset_autoconnect_portal() {
    Serial.println("[TEST]   Testing complete flow: reset -> autoConnect -> portal...");
    
    WiFiManager wm;
    
    // 1. Reset settings
    delay(100); // Ensure WiFi is in a stable state
    wm.resetSettings();
    delay(100); // Allow WiFi operations to complete
    
    // 2. autoConnect (should fail and start portal)
    wm.setConfigPortalTimeout(5);
    bool result = wm.autoConnect("TestAP");
    
    // 3. Verify portal started
    TEST_ASSERT_FALSE(result); // Connection failed
    TEST_ASSERT_TRUE(wm.getConfigPortalActive()); // Portal started
    
    // 4. Process portal
    wm.process();
    delay(100);
    
    // 5. Verify portal is still active
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // 6. Stop portal
    wm.stopConfigPortal();
    wm.process();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Complete flow: reset -> autoConnect -> portal test completed successfully");
}

// Test portal with parameters and infrastructure
void test_portal_with_parameters_and_infrastructure() {
    Serial.println("[TEST]   Testing portal with parameters and infrastructure...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Add parameters
    WiFiManagerParameter p1("server", "Server", "192.168.1.1", 40);
    WiFiManagerParameter p2("port", "Port", "1883", 6);
    wm.addParameter(&p1);
    wm.addParameter(&p2);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Verify portal is active
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Verify infrastructure
    TEST_ASSERT_NOT_NULL(wm.server);
    TEST_ASSERT_NOT_NULL(wm.dnsServer);
    
    // Verify parameters
    TEST_ASSERT_EQUAL(2, wm.getParametersCount());
    
    // Verify WiFi mode
    WiFiMode_t mode = WiFi.getMode();
    TEST_ASSERT_TRUE((mode & WIFI_AP) != 0);
    
    // Verify AP IP
    IPAddress apIP = WiFi.softAPIP();
    TEST_ASSERT_NOT_EQUAL(IPAddress(0,0,0,0), apIP);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Portal with parameters and infrastructure test completed successfully");
}

