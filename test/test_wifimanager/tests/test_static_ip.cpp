#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test static IP configuration
void test_set_ap_static_ip_config() {
    Serial.println("[TEST]   Testing setAPStaticIPConfig()...");
    
    WiFiManager wm;
    
    IPAddress ip(192, 168, 4, 1);
    IPAddress gw(192, 168, 4, 1);
    IPAddress sn(255, 255, 255, 0);
    
    // Set AP static IP
    wm.setAPStaticIPConfig(ip, gw, sn);
    
    // Verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "setAPStaticIPConfig() executed without crash");
    
    // Start portal and verify IP is set
    // Use non-blocking mode
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(10);
    
    // Note: In non-blocking mode, startConfigPortal() returns false even when successful
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    delay(200); // Give time for IP to be configured
    
    IPAddress apIP = WiFi.softAPIP();
    TEST_ASSERT_EQUAL(ip, apIP);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   setAPStaticIPConfig() test completed successfully");
}

void test_set_sta_static_ip_config() {
    Serial.println("[TEST]   Testing setSTAStaticIPConfig()...");
    
    WiFiManager wm;
    
    IPAddress ip(192, 168, 1, 100);
    IPAddress gw(192, 168, 1, 1);
    IPAddress sn(255, 255, 255, 0);
    
    // Set STA static IP
    wm.setSTAStaticIPConfig(ip, gw, sn);
    
    // Verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "setSTAStaticIPConfig() executed without crash");
    
    Serial.println("[TEST]   setSTAStaticIPConfig() test completed successfully");
}

void test_set_sta_static_ip_config_with_dns() {
    Serial.println("[TEST]   Testing setSTAStaticIPConfig() with DNS...");
    
    WiFiManager wm;
    
    IPAddress ip(192, 168, 1, 100);
    IPAddress gw(192, 168, 1, 1);
    IPAddress sn(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);
    
    // Set STA static IP with DNS
    wm.setSTAStaticIPConfig(ip, gw, sn, dns);
    
    // Verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "setSTAStaticIPConfig() with DNS executed without crash");
    
    Serial.println("[TEST]   setSTAStaticIPConfig() with DNS test completed successfully");
}

void test_show_static_fields() {
    Serial.println("[TEST]   Testing setShowStaticFields()...");
    
    WiFiManager wm;
    
    // Test enabling static fields
    wm.setShowStaticFields(true);
    
    // Test disabling static fields
    wm.setShowStaticFields(false);
    
    // Verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowStaticFields() executed without crash");
    
    Serial.println("[TEST]   setShowStaticFields() test completed successfully");
}

void test_show_dns_fields() {
    Serial.println("[TEST]   Testing setShowDnsFields()...");
    
    WiFiManager wm;
    
    // Test enabling DNS fields
    wm.setShowDnsFields(true);
    
    // Test disabling DNS fields
    wm.setShowDnsFields(false);
    
    // Verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowDnsFields() executed without crash");
    
    Serial.println("[TEST]   setShowDnsFields() test completed successfully");
}

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

