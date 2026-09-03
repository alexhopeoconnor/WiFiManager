#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <Arduino.h>

// Test function type
using TestFn = void(*)();

// Test case structure
struct TestCase {
    const char* name;
    TestFn fn;
    uint16_t line;
};

#define TEST_ENTRY(fn) { #fn, fn, __LINE__ }

// Basic tests
void test_basic_wifimanager_instantiation();
void test_reset_settings();

// Configuration tests
void test_set_and_get_hostname();
void test_get_default_ap_name();
void test_get_wifi_status_string();
void test_get_mode_string();
void test_get_rssi_as_quality();

// Portal lifecycle tests
void test_start_config_portal();
void test_start_config_portal_with_password();
void test_start_config_portal_auto_name();
void test_stop_config_portal();
void test_config_portal_infrastructure();
void test_start_web_portal();
void test_stop_web_portal();
void test_config_portal_multiple_start_stop();
void test_config_portal_already_active();
void test_get_config_portal_ssid();
void test_bootstrap_json_portal_feature_flags();
void test_portal_default_presentation();
void test_bootstrap_json_contract_v3();
void test_bootstrap_json_snapshot_consistency();
void test_root_render_interleaved_context_isolation();
void test_portal_presentation_configuration();

// Non-blocking tests
void test_nonblocking_process();
void test_client_check_setters();
void test_process_required_for_timeout();

// Callback tests
void test_ap_callback();
void test_web_server_callback();
void test_multiple_callbacks();

// Parameter tests
void test_create_parameter();
void test_add_parameter();
void test_get_parameters();
void test_parameter_with_custom_html();
void test_parameter_value_length();
void test_parameter_label_placement();
void test_multiple_parameters();
void test_parameter_placeholder();

// Static IP tests
void test_set_ap_static_ip_config();
void test_ap_static_ip_application();

// Advanced options tests

// WiFi connection tests
void test_autoconnect_fallback_to_portal();
void test_connectwifi_ssid_not_found();
void test_connection_state_transitions();
void test_autoconnect_with_timeout();

// Callback firing tests
void test_timeout_callback_fires();
void test_config_reset_callback_fires();
void test_ap_callback_fires_improved();
void test_web_server_callback_fires_improved();

// DNS lifecycle tests
void test_dns_server_created();
void test_dns_server_cleanup();
void test_dns_server_lifecycle_cycles();

// Error condition tests
void test_connection_failure_handling();
void test_resource_cleanup_after_error();
void test_multiple_rapid_start_stop();

// Parameter value tests
void test_parameter_value_set_directly();
void test_parameter_value_length_validation();
void test_parameter_value_persistence();
void test_parameter_value_update();
void test_multiple_parameters_different_values();

// WiFi scanning tests
void test_wifi_scan_initiates();
void test_async_scan_behavior();
void test_scan_status_checking();
void test_scan_completion_wait();
void test_scan_cancels_when_connect_pending();
void test_scan_cancels_when_lifecycle_blocked();
void test_scan_generation_invalidated_on_reset();

// Template rendering tests
void test_shell_template_renders_core_placeholders();
void test_shell_template_renders_dynamic_theme_with_percent_values();

// API JSON + captive helper tests
void test_captive_redirect_host_rule();
void test_api_wifi_meta_json_shape();
void test_api_wifi_meta_password_field_type();
void test_api_wifi_connect_status_success_redirect();
void test_profile_portal_success_keeps_handoff_alive();
void test_api_info_json_shape();
void test_api_params_json_shape();
void test_api_status_json_shape();

// State transition tests
void test_portal_to_connected_transition();
void test_concurrent_operations();
void test_state_consistency_during_portal();
void test_state_transitions_multiple_cycles();

// Integration tests
void test_autoconnect_fallback_flow();
void test_portal_lifecycle_with_connection_attempt();
void test_parameter_add_and_retrieve();
void test_complete_flow_reset_autoconnect_portal();
void test_portal_with_parameters_and_infrastructure();

// Stress tests
void test_many_start_stop_cycles();
void test_long_running_portal();
void test_rapid_portal_start_stop();
void test_multiple_parameters_stress();
void test_portal_with_timeout_stress();

#endif // TEST_MAIN_H

