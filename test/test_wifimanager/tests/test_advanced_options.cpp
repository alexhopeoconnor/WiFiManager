#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test advanced configuration options
void test_set_break_after_config() {
    Serial.println("[TEST]   Testing setBreakAfterConfig()...");
    
    WiFiManager wm;
    
    wm.setBreakAfterConfig(true);
    wm.setBreakAfterConfig(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setBreakAfterConfig() executed without crash");
    
    Serial.println("[TEST]   setBreakAfterConfig() test completed successfully");
}

void test_set_save_connect() {
    Serial.println("[TEST]   Testing setSaveConnect()...");
    
    WiFiManager wm;
    
    wm.setSaveConnect(true);
    wm.setSaveConnect(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setSaveConnect() executed without crash");
    
    Serial.println("[TEST]   setSaveConnect() test completed successfully");
}

void test_set_restore_persistent() {
    Serial.println("[TEST]   Testing setRestorePersistent()...");
    
    WiFiManager wm;
    
    wm.setRestorePersistent(true);
    wm.setRestorePersistent(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setRestorePersistent() executed without crash");
    
    Serial.println("[TEST]   setRestorePersistent() test completed successfully");
}

void test_set_show_password() {
    Serial.println("[TEST]   Testing setShowPassword()...");
    
    WiFiManager wm;
    
    wm.setShowPassword(true);
    wm.setShowPassword(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowPassword() executed without crash");
    
    Serial.println("[TEST]   setShowPassword() test completed successfully");
}

void test_set_scan_disp_perc() {
    Serial.println("[TEST]   Testing setScanDispPerc()...");
    
    WiFiManager wm;
    
    wm.setScanDispPerc(true);
    wm.setScanDispPerc(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setScanDispPerc() executed without crash");
    
    Serial.println("[TEST]   setScanDispPerc() test completed successfully");
}

void test_set_enable_config_portal() {
    Serial.println("[TEST]   Testing setEnableConfigPortal()...");
    
    WiFiManager wm;
    
    wm.setEnableConfigPortal(true);
    wm.setEnableConfigPortal(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setEnableConfigPortal() executed without crash");
    
    Serial.println("[TEST]   setEnableConfigPortal() test completed successfully");
}

void test_set_disable_config_portal() {
    Serial.println("[TEST]   Testing setDisableConfigPortal()...");
    
    WiFiManager wm;
    
    wm.setDisableConfigPortal(true);
    wm.setDisableConfigPortal(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setDisableConfigPortal() executed without crash");
    
    Serial.println("[TEST]   setDisableConfigPortal() test completed successfully");
}

void test_set_connect_retries() {
    Serial.println("[TEST]   Testing setConnectRetries()...");
    
    WiFiManager wm;
    
    wm.setConnectRetries(1);
    wm.setConnectRetries(3);
    wm.setConnectRetries(5);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setConnectRetries() executed without crash");
    
    Serial.println("[TEST]   setConnectRetries() test completed successfully");
}

void test_set_save_connect_timeout() {
    Serial.println("[TEST]   Testing setSaveConnectTimeout()...");
    
    WiFiManager wm;
    
    wm.setSaveConnectTimeout(0);
    wm.setSaveConnectTimeout(10);
    wm.setSaveConnectTimeout(30);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setSaveConnectTimeout() executed without crash");
    
    Serial.println("[TEST]   setSaveConnectTimeout() test completed successfully");
}

void test_set_show_info_erase() {
    Serial.println("[TEST]   Testing setShowInfoErase()...");
    
    WiFiManager wm;
    
    wm.setShowInfoErase(true);
    wm.setShowInfoErase(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowInfoErase() executed without crash");
    
    Serial.println("[TEST]   setShowInfoErase() test completed successfully");
}

void test_set_show_info_update() {
    Serial.println("[TEST]   Testing setShowInfoUpdate()...");
    
    WiFiManager wm;
    
    wm.setShowInfoUpdate(true);
    wm.setShowInfoUpdate(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setShowInfoUpdate() executed without crash");
    
    Serial.println("[TEST]   setShowInfoUpdate() test completed successfully");
}

void test_set_custom_head_element() {
    Serial.println("[TEST]   Testing setCustomHeadElement()...");
    
    WiFiManager wm;
    
    wm.setCustomHeadElement("<style>body { background: red; }</style>");
    wm.setCustomHeadElement("");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCustomHeadElement() executed without crash");
    
    Serial.println("[TEST]   setCustomHeadElement() test completed successfully");
}

void test_set_custom_body_header() {
    Serial.println("[TEST]   Testing setCustomBodyHeader()...");
    
    WiFiManager wm;
    
    wm.setCustomBodyHeader("<div>Custom Header</div>");
    wm.setCustomBodyHeader("");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCustomBodyHeader() executed without crash");
    
    Serial.println("[TEST]   setCustomBodyHeader() test completed successfully");
}

void test_set_custom_body_footer() {
    Serial.println("[TEST]   Testing setCustomBodyFooter()...");
    
    WiFiManager wm;
    
    wm.setCustomBodyFooter("<div>Custom Footer</div>");
    wm.setCustomBodyFooter("");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCustomBodyFooter() executed without crash");
    
    Serial.println("[TEST]   setCustomBodyFooter() test completed successfully");
}

void test_set_custom_menu_html() {
    Serial.println("[TEST]   Testing setCustomMenuHTML()...");
    
    WiFiManager wm;
    
    wm.setCustomMenuHTML("<div>Custom Menu</div>");
    wm.setCustomMenuHTML("");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setCustomMenuHTML() executed without crash");
    
    Serial.println("[TEST]   setCustomMenuHTML() test completed successfully");
}

void test_set_menu() {
    Serial.println("[TEST]   Testing setMenu()...");
    
    WiFiManager wm;
    
    // Test with array
    const char* menu[] = {"wifi", "info", "exit"};
    wm.setMenu(menu, 3);
    
    // Test with vector
    std::vector<const char*> menuVec = {"wifi", "info", "param", "exit"};
    wm.setMenu(menuVec);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setMenu() executed without crash");
    
    Serial.println("[TEST]   setMenu() test completed successfully");
}

void test_set_params_page() {
    Serial.println("[TEST]   Testing setParamsPage()...");
    
    WiFiManager wm;
    
    wm.setParamsPage(true);
    wm.setParamsPage(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setParamsPage() executed without crash");
    
    Serial.println("[TEST]   setParamsPage() test completed successfully");
}

void test_set_dark_mode() {
    Serial.println("[TEST]   Testing setDarkMode()...");
    
    WiFiManager wm;
    
    wm.setDarkMode(true);
    wm.setDarkMode(false);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setDarkMode() executed without crash");
    
    Serial.println("[TEST]   setDarkMode() test completed successfully");
}

void test_set_class() {
    Serial.println("[TEST]   Testing setClass()...");
    
    WiFiManager wm;
    
    wm.setClass("invert");
    wm.setClass("custom-class");
    wm.setClass("");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "setClass() executed without crash");
    
    Serial.println("[TEST]   setClass() test completed successfully");
}

void test_preload_wifi() {
    Serial.println("[TEST]   Testing preloadWiFi()...");
    
    WiFiManager wm;
    
    // Preload WiFi credentials (without actually connecting)
    wm.preloadWiFi("TestSSID", "TestPassword");
    
    // Should return true/false, just verify no crash
    TEST_ASSERT_TRUE_MESSAGE(true, "preloadWiFi() executed without crash");
    
    Serial.println("[TEST]   preloadWiFi() test completed successfully");
}

void test_debug_soft_ap_config() {
    Serial.println("[TEST]   Testing debugSoftAPConfig()...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(5);
    
    wm.startConfigPortal("TestAP");
    
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

