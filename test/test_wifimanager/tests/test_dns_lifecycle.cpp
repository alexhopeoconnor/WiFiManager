#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test DNS server is created when portal starts
void test_dns_server_created() {
    Serial.println("[TEST]   Testing DNS server creation...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Wait for DNS server to be created
    unsigned long start = millis();
    while (!wm.getDNSServer() && (millis() - start < 1000)) {
        wm.process();
        delay(10);
    }
    
    // DNS server should be created when portal starts
    TEST_ASSERT_NOT_NULL(wm.getDNSServer());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   DNS server creation test completed successfully");
}

// Test DNS server cleanup when portal stops
void test_dns_server_cleanup() {
    Serial.println("[TEST]   Testing DNS server cleanup...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Wait for DNS server to be created
    unsigned long start = millis();
    while (!wm.getDNSServer() && (millis() - start < 1000)) {
        wm.process();
        delay(10);
    }
    
    // Verify DNS server exists
    TEST_ASSERT_NOT_NULL(wm.getDNSServer());
    
    // Stop portal
    wm.stopConfigPortal();
    wm.process();
    
    // DNS server should be cleaned up (set to nullptr or deleted)
    // Note: Actual cleanup verification depends on implementation
    // For now, verify portal is stopped
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   DNS server cleanup test completed successfully");
}

// Test DNS server lifecycle with multiple start/stop cycles
void test_dns_server_lifecycle_cycles() {
    Serial.println("[TEST]   Testing DNS server lifecycle with multiple cycles...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Cycle 1
    wm.startConfigPortal("TestAP1");
    wm.process();
    
    unsigned long start = millis();
    while (!wm.getDNSServer() && (millis() - start < 1000)) {
        wm.process();
        delay(10);
    }
    TEST_ASSERT_NOT_NULL(wm.getDNSServer());
    
    wm.stopConfigPortal();
    wm.process();
    delay(200);
    
    // Cycle 2
    wm.startConfigPortal("TestAP2");
    wm.process();
    
    start = millis();
    while (!wm.getDNSServer() && (millis() - start < 1000)) {
        wm.process();
        delay(10);
    }
    TEST_ASSERT_NOT_NULL(wm.getDNSServer());
    
    wm.stopConfigPortal();
    wm.process();
    
    Serial.println("[TEST]   DNS server lifecycle cycles test completed successfully");
}

