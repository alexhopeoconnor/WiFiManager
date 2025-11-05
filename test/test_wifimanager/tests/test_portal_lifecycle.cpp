#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test config portal lifecycle
void test_start_config_portal() {
    Serial.println("[TEST]   Testing startConfigPortal()...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // Start portal with SSID only
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   startConfigPortal() test completed successfully");
}

void test_start_config_portal_with_password() {
    Serial.println("[TEST]   Testing startConfigPortal() with password...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // Start portal with SSID and password
    wm.startConfigPortal("TestAP2", "password123");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   startConfigPortal() with password test completed successfully");
}

void test_start_config_portal_auto_name() {
    Serial.println("[TEST]   Testing startConfigPortal() with auto-generated name...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // Start portal without SSID (uses chip ID)
    wm.startConfigPortal();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100); // Give it time to initialize
    
    // Verify we can get the SSID
    String ssid = wm.getConfigPortalSSID();
    TEST_ASSERT_GREATER_THAN(0, ssid.length());
    
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   startConfigPortal() auto-name test completed successfully");
}

void test_stop_config_portal() {
    Serial.println("[TEST]   Testing stopConfigPortal()...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    // Stop portal
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   stopConfigPortal() test completed successfully");
}

void test_config_portal_infrastructure() {
    Serial.println("[TEST]   Testing config portal infrastructure...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(200); // Give time for initialization
    
    // Verify server object exists (must check while portal is active)
    TEST_ASSERT_NOT_NULL_MESSAGE(wm.server, "Server object should exist when portal is active");
    
    // Verify DNS server exists
    TEST_ASSERT_NOT_NULL_MESSAGE(wm.dnsServer, "DNS server object should exist when portal is active");
    
    // Verify WiFi is in AP mode
    WiFiMode_t mode = WiFi.getMode();
    TEST_ASSERT_TRUE_MESSAGE((mode & WIFI_AP) != 0, "WiFi should be in AP mode");
    
    // Verify AP has an IP
    IPAddress apIP = WiFi.softAPIP();
    TEST_ASSERT_NOT_EQUAL(IPAddress(0,0,0,0), apIP);
    
    // Verify AP SSID matches
    String apSSID = WiFi.softAPSSID();
    TEST_ASSERT_EQUAL_STRING("TestAP", apSSID.c_str());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Config portal infrastructure test completed successfully");
}

void test_start_web_portal() {
    Serial.println("[TEST]   Testing startWebPortal()...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    // Note: Web portal requires STA mode, but for testing we can just verify it starts
    // In real usage, WiFi would need to be connected first
    wm.startWebPortal();
    
    TEST_ASSERT_TRUE(wm.getWebPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    
    // Verify server object exists
    TEST_ASSERT_NOT_NULL(wm.server);
    
    wm.stopWebPortal();
    
    Serial.println("[TEST]   startWebPortal() test completed successfully");
}

void test_stop_web_portal() {
    Serial.println("[TEST]   Testing stopWebPortal()...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    wm.startWebPortal();
    TEST_ASSERT_TRUE(wm.getWebPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    wm.stopWebPortal();
    TEST_ASSERT_FALSE(wm.getWebPortalActive());
    
    Serial.println("[TEST]   stopWebPortal() test completed successfully");
}

void test_config_portal_multiple_start_stop() {
    Serial.println("[TEST]   Testing multiple config portal start/stop cycles...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // First cycle
    wm.startConfigPortal("TestAP1");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    wm.process();
    delay(100);
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    delay(200);
    
    // Second cycle
    wm.startConfigPortal("TestAP2");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    wm.process();
    delay(100);
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    delay(200);
    
    // Third cycle
    wm.startConfigPortal("TestAP3");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    wm.process();
    delay(100);
    wm.stopConfigPortal();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Multiple config portal cycles test completed successfully");
}

void test_config_portal_already_active() {
    Serial.println("[TEST]   Testing startConfigPortal() when already active...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // Start portal
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    // Try to start again (should silently fail when already active)
    wm.startConfigPortal("TestAP2");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive()); // Still active with original AP name
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Config portal already active test completed successfully");
}

void test_get_config_portal_ssid() {
    Serial.println("[TEST]   Testing getConfigPortalSSID()...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    
    // Start with known SSID
    wm.startConfigPortal("MyTestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    // Verify SSID matches what we set
    String ssid = wm.getConfigPortalSSID();
    TEST_ASSERT_EQUAL_STRING("MyTestAP", ssid.c_str());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   getConfigPortalSSID() test completed successfully");
}

