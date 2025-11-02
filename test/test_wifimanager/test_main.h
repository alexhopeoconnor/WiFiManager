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
void test_disconnect();

// Configuration tests
void test_configuration_setters();
void test_set_and_get_hostname();
void test_wifi_ap_configuration_setters();
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

// Non-blocking tests
void test_nonblocking_process();
void test_client_check_setters();
void test_nonblocking_timeout_behavior();

// Callback tests
void test_ap_callback();
void test_web_server_callback();
void test_callback_registration();
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
void test_sta_static_ip_configuration();
void test_ap_static_ip_application();

// Advanced options tests
void test_connection_behavior_options();
void test_ui_display_options();
void test_ui_customization();
void test_debug_soft_ap_config();
void test_debug_platform_info();

#endif // TEST_MAIN_H

