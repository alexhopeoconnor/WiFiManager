#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "test_main.h"

// Test case array
TestCase tests[] = {
    // Basic tests
    TEST_ENTRY(test_basic_wifimanager_instantiation),
    TEST_ENTRY(test_reset_settings),
    // JSON API shape (buildApiInfoJson is large — run with fresh heap, before stress tests)
    TEST_ENTRY(test_captive_redirect_host_rule),
    TEST_ENTRY(test_api_wifi_meta_json_shape),
    TEST_ENTRY(test_api_wifi_meta_password_field_type),
    TEST_ENTRY(test_api_wifi_connect_status_success_redirect),
    TEST_ENTRY(test_profile_portal_success_keeps_handoff_alive),
    TEST_ENTRY(test_api_info_json_shape),
    TEST_ENTRY(test_api_params_json_shape),
    TEST_ENTRY(test_api_status_json_shape),
    
    // Configuration tests
    TEST_ENTRY(test_set_and_get_hostname),
    TEST_ENTRY(test_get_default_ap_name),
    TEST_ENTRY(test_get_wifi_status_string),
    TEST_ENTRY(test_get_mode_string),
    TEST_ENTRY(test_get_rssi_as_quality),
    
    // Portal lifecycle tests
    TEST_ENTRY(test_start_config_portal),
    TEST_ENTRY(test_start_config_portal_with_password),
    TEST_ENTRY(test_start_config_portal_auto_name),
    TEST_ENTRY(test_stop_config_portal),
    TEST_ENTRY(test_config_portal_infrastructure),
    TEST_ENTRY(test_start_web_portal),
    TEST_ENTRY(test_stop_web_portal),
    TEST_ENTRY(test_config_portal_multiple_start_stop),
    TEST_ENTRY(test_config_portal_already_active),
    TEST_ENTRY(test_get_config_portal_ssid),
    TEST_ENTRY(test_bootstrap_json_portal_feature_flags),
    TEST_ENTRY(test_portal_default_presentation),
    TEST_ENTRY(test_bootstrap_json_contract_v3),
    TEST_ENTRY(test_bootstrap_json_snapshot_consistency),
    TEST_ENTRY(test_root_render_interleaved_context_isolation),
    TEST_ENTRY(test_portal_presentation_configuration),
    
    // Non-blocking tests
    TEST_ENTRY(test_nonblocking_process),
    TEST_ENTRY(test_client_check_setters),
    TEST_ENTRY(test_process_required_for_timeout),
    
    // Callback tests
    TEST_ENTRY(test_ap_callback),
    TEST_ENTRY(test_web_server_callback),
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
    TEST_ENTRY(test_ap_static_ip_application),
    
    // WiFi connection tests
    TEST_ENTRY(test_autoconnect_fallback_to_portal),
    TEST_ENTRY(test_connectwifi_ssid_not_found),
    TEST_ENTRY(test_connection_state_transitions),
    TEST_ENTRY(test_autoconnect_with_timeout),
    
    // Callback firing tests
    TEST_ENTRY(test_timeout_callback_fires),
    TEST_ENTRY(test_config_reset_callback_fires),
    TEST_ENTRY(test_ap_callback_fires_improved),
    TEST_ENTRY(test_web_server_callback_fires_improved),
    
    // DNS lifecycle tests
    TEST_ENTRY(test_dns_server_created),
    TEST_ENTRY(test_dns_server_cleanup),
    TEST_ENTRY(test_dns_server_lifecycle_cycles),
    
    // Error condition tests
    TEST_ENTRY(test_connection_failure_handling),
    TEST_ENTRY(test_resource_cleanup_after_error),
    TEST_ENTRY(test_multiple_rapid_start_stop),
    
    // Parameter value tests
    TEST_ENTRY(test_parameter_value_set_directly),
    TEST_ENTRY(test_parameter_value_length_validation),
    TEST_ENTRY(test_parameter_value_persistence),
    TEST_ENTRY(test_parameter_value_update),
    TEST_ENTRY(test_multiple_parameters_different_values),
    
    // WiFi scanning tests
    TEST_ENTRY(test_wifi_scan_initiates),
    TEST_ENTRY(test_async_scan_behavior),
    TEST_ENTRY(test_scan_status_checking),
    TEST_ENTRY(test_scan_completion_wait),
    TEST_ENTRY(test_scan_cancels_when_connect_pending),
    TEST_ENTRY(test_scan_cancels_when_lifecycle_blocked),
    TEST_ENTRY(test_scan_generation_invalidated_on_reset),

    // Template rendering tests
    TEST_ENTRY(test_shell_template_renders_core_placeholders),
    TEST_ENTRY(test_shell_template_renders_dynamic_theme_with_percent_values),

    // State transition tests
    TEST_ENTRY(test_portal_to_connected_transition),
    TEST_ENTRY(test_concurrent_operations),
    TEST_ENTRY(test_state_consistency_during_portal),
    TEST_ENTRY(test_state_transitions_multiple_cycles),
    
    // Integration tests
    TEST_ENTRY(test_autoconnect_fallback_flow),
    TEST_ENTRY(test_portal_lifecycle_with_connection_attempt),
    TEST_ENTRY(test_parameter_add_and_retrieve),
    TEST_ENTRY(test_complete_flow_reset_autoconnect_portal),
    TEST_ENTRY(test_portal_with_parameters_and_infrastructure),
    
    // Stress tests
    TEST_ENTRY(test_many_start_stop_cycles),
    TEST_ENTRY(test_long_running_portal),
    TEST_ENTRY(test_rapid_portal_start_stop),
    TEST_ENTRY(test_multiple_parameters_stress),
    TEST_ENTRY(test_portal_with_timeout_stress),
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

