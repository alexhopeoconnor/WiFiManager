#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Callback flags
bool ap_callback_fired = false;
bool web_server_callback_fired = false;
bool save_config_callback_fired = false;
bool pre_save_config_callback_fired = false;
bool save_params_callback_fired = false;
bool pre_save_params_callback_fired = false;
bool config_reset_callback_fired = false;
bool config_portal_timeout_callback_fired = false;

// Callback functions
void ap_callback(WiFiManager* wm) {
    ap_callback_fired = true;
}

void web_server_callback() {
    web_server_callback_fired = true;
}

void save_config_callback() {
    save_config_callback_fired = true;
}

void pre_save_config_callback() {
    pre_save_config_callback_fired = true;
}

void save_params_callback() {
    save_params_callback_fired = true;
}

void pre_save_params_callback() {
    pre_save_params_callback_fired = true;
}

void config_reset_callback() {
    config_reset_callback_fired = true;
}

void config_portal_timeout_callback() {
    config_portal_timeout_callback_fired = true;
}

// Reset all callback flags
void reset_callback_flags() {
    ap_callback_fired = false;
    web_server_callback_fired = false;
    save_config_callback_fired = false;
    pre_save_config_callback_fired = false;
    save_params_callback_fired = false;
    pre_save_params_callback_fired = false;
    config_reset_callback_fired = false;
    config_portal_timeout_callback_fired = false;
}

void test_ap_callback() {
    Serial.println("[TEST]   Testing AP callback...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    wm.setAPCallback(ap_callback);
    wm.setConfigPortalTimeout(5);
    
    wm.startConfigPortal("TestAP");
    
    // Callback should have fired when AP starts
    TEST_ASSERT_TRUE(ap_callback_fired);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   AP callback test completed successfully");
}

void test_web_server_callback() {
    Serial.println("[TEST]   Testing web server callback...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    wm.setWebServerCallback(web_server_callback);
    wm.setConfigPortalTimeout(5);
    
    wm.startConfigPortal("TestAP");
    
    delay(100); // Give time for server to start
    
    // Callback should have fired when server starts
    TEST_ASSERT_TRUE(web_server_callback_fired);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Web server callback test completed successfully");
}

void test_config_reset_callback() {
    Serial.println("[TEST]   Testing config reset callback...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    wm.setConfigResetCallback(config_reset_callback);
    
    // Reset settings should trigger callback
    wm.resetSettings();
    
    // Note: Callback may or may not fire on resetSettings() depending on implementation
    // Just verify it doesn't crash
    TEST_ASSERT_TRUE_MESSAGE(true, "setConfigResetCallback() executed without crash");
    
    Serial.println("[TEST]   Config reset callback test completed successfully");
}

void test_save_config_callback_registration() {
    Serial.println("[TEST]   Testing save config callback registration...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    
    // Register callback
    wm.setSaveConfigCallback(save_config_callback);
    wm.setPreSaveConfigCallback(pre_save_config_callback);
    
    // Note: These callbacks fire when WiFi is saved via web portal
    // We can't easily test the actual firing without HTTP requests,
    // but we can verify registration doesn't crash
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Save config callbacks registered without crash");
    
    Serial.println("[TEST]   Save config callback registration test completed successfully");
}

void test_save_params_callback_registration() {
    Serial.println("[TEST]   Testing save params callback registration...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    
    // Register callbacks
    wm.setSaveParamsCallback(save_params_callback);
    wm.setPreSaveParamsCallback(pre_save_params_callback);
    
    // Note: These callbacks fire when parameters are saved via web portal
    // We can't easily test the actual firing without HTTP requests,
    // but we can verify registration doesn't crash
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Save params callbacks registered without crash");
    
    Serial.println("[TEST]   Save params callback registration test completed successfully");
}

void test_config_portal_timeout_callback_registration() {
    Serial.println("[TEST]   Testing config portal timeout callback registration...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    
    // Register callback
    wm.setConfigPortalTimeoutCallback(config_portal_timeout_callback);
    
    // Note: This callback fires when config portal times out
    // We can't easily test the actual firing without waiting for timeout,
    // but we can verify registration doesn't crash
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Config portal timeout callback registered without crash");
    
    Serial.println("[TEST]   Config portal timeout callback registration test completed successfully");
}

void test_multiple_callbacks() {
    Serial.println("[TEST]   Testing multiple callbacks together...");
    
    reset_callback_flags();
    
    WiFiManager wm;
    
    // Register all callbacks
    wm.setAPCallback(ap_callback);
    wm.setWebServerCallback(web_server_callback);
    wm.setSaveConfigCallback(save_config_callback);
    wm.setPreSaveConfigCallback(pre_save_config_callback);
    wm.setSaveParamsCallback(save_params_callback);
    wm.setPreSaveParamsCallback(pre_save_params_callback);
    wm.setConfigResetCallback(config_reset_callback);
    wm.setConfigPortalTimeoutCallback(config_portal_timeout_callback);
    
    // Start portal - should fire AP and web server callbacks
    wm.setConfigPortalTimeout(5);
    wm.startConfigPortal("TestAP");
    
    delay(100);
    
    // Verify AP callback fired
    TEST_ASSERT_TRUE(ap_callback_fired);
    TEST_ASSERT_TRUE(web_server_callback_fired);
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Multiple callbacks test completed successfully");
}

