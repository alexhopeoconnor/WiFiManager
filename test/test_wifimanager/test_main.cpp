#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "test_main.h"

// Test case array
TestCase tests[] = {
    // Basic tests
    TEST_ENTRY(test_basic_wifimanager_instantiation),
    TEST_ENTRY(test_reset_settings),
    TEST_ENTRY(test_disconnect),
    
    // Configuration tests
    TEST_ENTRY(test_set_title),
    TEST_ENTRY(test_set_config_portal_timeout),
    TEST_ENTRY(test_set_connect_timeout),
    TEST_ENTRY(test_set_http_port),
    TEST_ENTRY(test_set_minimum_signal_quality),
    TEST_ENTRY(test_set_remove_duplicate_aps),
    TEST_ENTRY(test_set_show_static_fields),
    TEST_ENTRY(test_set_show_dns_fields),
    TEST_ENTRY(test_set_config_portal_blocking),
    TEST_ENTRY(test_set_captive_portal_enable),
    TEST_ENTRY(test_set_hostname),
    TEST_ENTRY(test_set_wifi_ap_channel),
    TEST_ENTRY(test_set_wifi_ap_hidden),
    TEST_ENTRY(test_set_clean_connect),
    TEST_ENTRY(test_set_country),
    TEST_ENTRY(test_set_wifi_auto_reconnect),
    TEST_ENTRY(test_get_default_ap_name),
    TEST_ENTRY(test_get_wifi_status_string),
    TEST_ENTRY(test_get_mode_string),
    TEST_ENTRY(test_get_rssi_as_quality),
    
    // Portal lifecycle tests
    TEST_ENTRY(test_start_config_portal),
    TEST_ENTRY(test_start_config_portal_auto_name),
    TEST_ENTRY(test_stop_config_portal),
    TEST_ENTRY(test_config_portal_infrastructure),
    TEST_ENTRY(test_start_web_portal),
    TEST_ENTRY(test_stop_web_portal),
    TEST_ENTRY(test_config_portal_multiple_start_stop),
    TEST_ENTRY(test_config_portal_already_active),
    TEST_ENTRY(test_get_config_portal_ssid),
    
    // Non-blocking tests
    TEST_ENTRY(test_nonblocking_mode_setup),
    TEST_ENTRY(test_nonblocking_process),
    TEST_ENTRY(test_ap_client_check),
    TEST_ENTRY(test_web_portal_client_check),
    TEST_ENTRY(test_nonblocking_timeout_behavior),
    
    // Callback tests
    TEST_ENTRY(test_ap_callback),
    TEST_ENTRY(test_web_server_callback),
    TEST_ENTRY(test_config_reset_callback),
    TEST_ENTRY(test_save_config_callback_registration),
    TEST_ENTRY(test_save_params_callback_registration),
    TEST_ENTRY(test_config_portal_timeout_callback_registration),
    TEST_ENTRY(test_multiple_callbacks),
    
    // Parameter tests
    TEST_ENTRY(test_create_parameter),
    TEST_ENTRY(test_add_parameter),
    TEST_ENTRY(test_get_parameters),
    TEST_ENTRY(test_parameter_with_custom_html),
    TEST_ENTRY(test_parameter_value_length),
    TEST_ENTRY(test_parameter_label_placement),
    TEST_ENTRY(test_multiple_parameters),
    TEST_ENTRY(test_parameter_placeholder),
    
    // Static IP tests
    TEST_ENTRY(test_set_ap_static_ip_config),
    TEST_ENTRY(test_set_sta_static_ip_config),
    TEST_ENTRY(test_set_sta_static_ip_config_with_dns),
    TEST_ENTRY(test_show_static_fields),
    TEST_ENTRY(test_show_dns_fields),
    TEST_ENTRY(test_ap_static_ip_application),
    
    // Advanced options tests
    TEST_ENTRY(test_set_break_after_config),
    TEST_ENTRY(test_set_save_connect),
    TEST_ENTRY(test_set_restore_persistent),
    TEST_ENTRY(test_set_show_password),
    TEST_ENTRY(test_set_scan_disp_perc),
    TEST_ENTRY(test_set_enable_config_portal),
    TEST_ENTRY(test_set_disable_config_portal),
    TEST_ENTRY(test_set_connect_retries),
    TEST_ENTRY(test_set_save_connect_timeout),
    TEST_ENTRY(test_set_show_info_erase),
    TEST_ENTRY(test_set_show_info_update),
    TEST_ENTRY(test_set_custom_head_element),
    TEST_ENTRY(test_set_custom_body_header),
    TEST_ENTRY(test_set_custom_body_footer),
    TEST_ENTRY(test_set_custom_menu_html),
    TEST_ENTRY(test_set_menu),
    TEST_ENTRY(test_set_params_page),
    TEST_ENTRY(test_set_dark_mode),
    TEST_ENTRY(test_set_class),
    TEST_ENTRY(test_preload_wifi),
    TEST_ENTRY(test_debug_soft_ap_config),
    TEST_ENTRY(test_debug_platform_info),
};

const size_t TEST_COUNT = sizeof(tests) / sizeof(TestCase);

// Test state variables
size_t next_index = 0;
bool begun = false;

// Test setup and teardown
void setUp(void) {
    // No per-test setup needed
}

void tearDown(void) {
    // No per-test teardown needed
}

// Arduino setup and loop functions (required by framework)
void setup() {
    Serial.begin(115200);
    delay(2000); // Give time for serial to initialize
    
    Serial.println("\n[TEST] =============================================");
    Serial.println("[TEST] === WiFiManager Test Setup ===");
    Serial.println("[TEST] Starting WiFiManager tests...");
    Serial.println("[TEST] =============================================");
    
    UNITY_BEGIN(); // Start Unity test framework
    begun = true; // Start tests immediately
}

void loop() {
    // Run one test and return immediately
    if (begun && next_index < TEST_COUNT) {
        TestCase& t = tests[next_index];
        Serial.print("\n[TEST] ==== Running test: ");
        Serial.print(t.name);
        Serial.println(" ====");
        UnityDefaultTestRun(t.fn, t.name, t.line);
        next_index++;
        return; // yield quickly
    }
    
    // All tests completed
    if (begun && next_index >= TEST_COUNT) {
        Serial.println("\n[TEST] =============================================");
        Serial.println("[TEST] === All tests completed ===");
        Serial.println("[TEST] =============================================");
        UNITY_END(); // prints Unity summary
        begun = false; // avoid repeating
    }
}

