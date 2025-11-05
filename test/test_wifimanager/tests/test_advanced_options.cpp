#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test advanced configuration options - consolidated into logical groups

// Test connection behavior options
void test_connection_behavior_options() {
    Serial.println("[TEST]   Testing connection behavior options...");
    
    WiFiManager wm;
    
    // Test connection options
    wm.setBreakAfterConfig(true);
    wm.setBreakAfterConfig(false);
    wm.setSaveConnect(true);
    wm.setSaveConnect(false);
    wm.setRestorePersistent(true);
    wm.setRestorePersistent(false);
    wm.setConnectRetries(1);
    wm.setConnectRetries(3);
    wm.setConnectRetries(5);
    wm.setSaveConnectTimeout(0);
    wm.setSaveConnectTimeout(10);
    wm.setSaveConnectTimeout(30);
    wm.setEnableConfigPortal(true);
    wm.setEnableConfigPortal(false);
    wm.setDisableConfigPortal(true);
    wm.setDisableConfigPortal(false);
    wm.preloadWiFi("TestSSID", "TestPassword");
    
    // All setters executed without crash
    TEST_ASSERT_TRUE_MESSAGE(true, "Connection behavior options executed without crash");
    
    Serial.println("[TEST]   Connection behavior options test completed successfully");
}

// Test UI display options
void test_ui_display_options() {
    Serial.println("[TEST]   Testing UI display options...");
    
    WiFiManager wm;
    
    // Test display toggles
    wm.setShowPassword(true);
    wm.setShowPassword(false);
    wm.setScanDispPerc(true);
    wm.setScanDispPerc(false);
    wm.setShowInfoErase(true);
    wm.setShowInfoErase(false);
    wm.setShowInfoUpdate(true);
    wm.setShowInfoUpdate(false);
    wm.setDarkMode(true);
    wm.setDarkMode(false);
    wm.setParamsPage(true);
    wm.setParamsPage(false);
    
    // All setters executed without crash
    TEST_ASSERT_TRUE_MESSAGE(true, "UI display options executed without crash");
    
    Serial.println("[TEST]   UI display options test completed successfully");
}

// Test custom HTML/UI customization
void test_ui_customization() {
    Serial.println("[TEST]   Testing UI customization...");
    
    WiFiManager wm;
    
    // Test custom HTML elements
    wm.setCustomHeadElement("<style>body { background: red; }</style>");
    wm.setCustomHeadElement("");
    wm.setCustomBodyHeader("<div>Custom Header</div>");
    wm.setCustomBodyHeader("");
    wm.setCustomBodyFooter("<div>Custom Footer</div>");
    wm.setCustomBodyFooter("");
    wm.setCustomMenuHTML("<div>Custom Menu</div>");
    wm.setCustomMenuHTML("");
    
    // Test menu configuration
    const char* menu[] = {"wifi", "info", "exit"};
    wm.setMenu(menu, 3);
    std::vector<const char*> menuVec = {"wifi", "info", "param", "exit"};
    wm.setMenu(menuVec);
    
    // Test CSS class
    wm.setClass("invert");
    wm.setClass("custom-class");
    wm.setClass("");
    
    // All setters executed without crash
    TEST_ASSERT_TRUE_MESSAGE(true, "UI customization options executed without crash");
    
    Serial.println("[TEST]   UI customization test completed successfully");
}

void test_debug_soft_ap_config() {
    Serial.println("[TEST]   Testing debugSoftAPConfig()...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    wm.startConfigPortal("TestAP");
    
    // Call process() to simulate real usage
    wm.process();
    delay(100);
    
    // Debug output should not crash
    wm.debugSoftAPConfig();
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   debugSoftAPConfig() test completed successfully");
}

void test_debug_platform_info() {
    Serial.println("[TEST]   Testing debugPlatformInfo()...");
    
    WiFiManager wm;
    
    // Debug output should not crash
    wm.debugPlatformInfo();
    
    TEST_ASSERT_TRUE_MESSAGE(true, "debugPlatformInfo() executed without crash");
    
    Serial.println("[TEST]   debugPlatformInfo() test completed successfully");
}

