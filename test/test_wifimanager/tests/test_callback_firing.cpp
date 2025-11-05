#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Callback flags (static to avoid duplicate definitions)
static bool timeout_callback_fired = false;
static bool config_reset_callback_fired = false;

// Callback functions (static to avoid duplicate definitions)
static void timeout_callback() {
    timeout_callback_fired = true;
}

static void config_reset_callback() {
    config_reset_callback_fired = true;
}

// Reset callback flags (static to avoid duplicate definitions)
static void reset_callback_flags() {
    timeout_callback_fired = false;
    config_reset_callback_fired = false;
}

// Test timeout callback fires
void test_timeout_callback_fires() {
    Serial.println("[TEST]   Testing timeout callback firing...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    wm.setConfigPortalTimeoutCallback(timeout_callback);
    
    wm.setConfigPortalTimeout(2); // 2 second timeout
    wm.startConfigPortal("TestAP");
    
    // Process until timeout
    unsigned long start = millis();
    while (wm.getConfigPortalActive() && (millis() - start < 5000)) {
        wm.process();
        delay(50);
    }
    
    // Callback should have fired when timeout occurred
    TEST_ASSERT_TRUE(timeout_callback_fired);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Timeout callback firing test completed successfully");
}

// Test config reset callback fires
void test_config_reset_callback_fires() {
    Serial.println("[TEST]   Testing config reset callback firing...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    wm.setConfigResetCallback(config_reset_callback);
    
    // Ensure WiFi is in a stable state before resetting
    delay(100);
    
    // Call resetSettings which should trigger callback
    wm.resetSettings();
    
    // Allow WiFi operations to complete
    delay(100);
    
    // Callback should have fired
    TEST_ASSERT_TRUE(config_reset_callback_fired);
    
    Serial.println("[TEST]   Config reset callback firing test completed successfully");
}

// Test AP callback fires (improve existing test)
void test_ap_callback_fires_improved() {
    Serial.println("[TEST]   Testing AP callback firing (improved)...");
    
    bool ap_callback_fired = false;
    
    WiFiManager wm;
    wm.setAPCallback([&](WiFiManager* wm) {
        ap_callback_fired = true;
        (void)wm; // Suppress unused parameter warning
    });
    
    wm.setConfigPortalTimeout(5);
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Callback should have fired when AP starts
    TEST_ASSERT_TRUE(ap_callback_fired);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   AP callback firing (improved) test completed successfully");
}

// Test web server callback fires (improve existing test)
void test_web_server_callback_fires_improved() {
    Serial.println("[TEST]   Testing web server callback firing (improved)...");
    
    bool web_server_callback_fired = false;
    
    WiFiManager wm;
    wm.setWebServerCallback([&]() {
        web_server_callback_fired = true;
    });
    
    wm.setConfigPortalTimeout(5);
    wm.startConfigPortal("TestAP");
    wm.process();
    
    // Wait for server to initialize
    unsigned long start = millis();
    while (!web_server_callback_fired && (millis() - start < 1000)) {
        wm.process();
        delay(10);
    }
    
    // Callback should have fired when server starts
    TEST_ASSERT_TRUE(web_server_callback_fired);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Web server callback firing (improved) test completed successfully");
}

