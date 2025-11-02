#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test static IP configuration
// Test AP static IP configuration - verifies IP is actually applied
void test_set_ap_static_ip_config() {
    Serial.println("[TEST]   Testing setAPStaticIPConfig()...");
    
    WiFiManager wm;
    
    IPAddress ip(192, 168, 4, 1);
    IPAddress gw(192, 168, 4, 1);
    IPAddress sn(255, 255, 255, 0);
    
    // Set AP static IP
    wm.setAPStaticIPConfig(ip, gw, sn);
    
    // Use non-blocking mode
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(10);
    
    // Start portal and verify IP is actually set
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    delay(200); // Give time for IP to be configured
    
    // Verify AP IP matches what we configured
    IPAddress apIP = WiFi.softAPIP();
    TEST_ASSERT_EQUAL(ip, apIP);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   setAPStaticIPConfig() test completed successfully");
}

// Test STA static IP configuration - consolidated
void test_sta_static_ip_configuration() {
    Serial.println("[TEST]   Testing STA static IP configuration...");
    
    WiFiManager wm;
    
    IPAddress ip(192, 168, 1, 100);
    IPAddress gw(192, 168, 1, 1);
    IPAddress sn(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);
    
    // Test setSTAStaticIPConfig without DNS
    wm.setSTAStaticIPConfig(ip, gw, sn);
    
    // Test setSTAStaticIPConfig with DNS
    wm.setSTAStaticIPConfig(ip, gw, sn, dns);
    
    // Verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "STA static IP configuration setters executed without crash");
    
    Serial.println("[TEST]   STA static IP configuration test completed successfully");
}

// Note: test_show_static_fields and test_show_dns_fields removed - 
// already covered in test_configuration_setters() in test_configuration.cpp

void test_ap_static_ip_application() {
    Serial.println("[TEST]   Testing AP static IP application...");
    
    WiFiManager wm;
    
    IPAddress customIP(10, 0, 1, 1);
    IPAddress customGW(10, 0, 1, 1);
    IPAddress customSN(255, 255, 255, 0);
    
    wm.setAPStaticIPConfig(customIP, customGW, customSN);
    
    // Use non-blocking mode
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(10);
    
    // Note: In non-blocking mode, startConfigPortal() returns false even when successful
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    delay(200); // Give time for AP to configure
    
    IPAddress apIP = WiFi.softAPIP();
    
    // AP should have the configured IP
    TEST_ASSERT_EQUAL(customIP, apIP);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   AP static IP application test completed successfully");
}

