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
void test_set_title();
void test_set_config_portal_timeout();
void test_set_connect_timeout();
void test_set_http_port();
void test_set_minimum_signal_quality();
void test_set_remove_duplicate_aps();
void test_set_show_static_fields();
void test_set_show_dns_fields();
void test_set_config_portal_blocking();
void test_set_captive_portal_enable();
void test_set_hostname();
void test_set_wifi_ap_channel();
void test_set_wifi_ap_hidden();
void test_set_clean_connect();
void test_set_country();
void test_set_wifi_auto_reconnect();
void test_get_default_ap_name();
void test_get_wifi_status_string();
void test_get_mode_string();
void test_get_rssi_as_quality();

// Portal lifecycle tests
void test_start_config_portal();
void test_start_config_portal_auto_name();
void test_stop_config_portal();
void test_config_portal_infrastructure();
void test_start_web_portal();
void test_stop_web_portal();
void test_config_portal_multiple_start_stop();
void test_config_portal_already_active();
void test_get_config_portal_ssid();

// Non-blocking tests
void test_nonblocking_mode_setup();
void test_nonblocking_process();
void test_ap_client_check();
void test_web_portal_client_check();
void test_nonblocking_timeout_behavior();

// Callback tests
void test_ap_callback();
void test_web_server_callback();
void test_config_reset_callback();
void test_save_config_callback_registration();
void test_save_params_callback_registration();
void test_config_portal_timeout_callback_registration();
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
void test_set_sta_static_ip_config();
void test_set_sta_static_ip_config_with_dns();
void test_show_static_fields();
void test_show_dns_fields();
void test_ap_static_ip_application();

// Advanced options tests
void test_set_break_after_config();
void test_set_save_connect();
void test_set_restore_persistent();
void test_set_show_password();
void test_set_scan_disp_perc();
void test_set_enable_config_portal();
void test_set_disable_config_portal();
void test_set_connect_retries();
void test_set_save_connect_timeout();
void test_set_show_info_erase();
void test_set_show_info_update();
void test_set_custom_head_element();
void test_set_custom_body_header();
void test_set_custom_body_footer();
void test_set_custom_menu_html();
void test_set_menu();
void test_set_params_page();
void test_set_dark_mode();
void test_set_class();
void test_preload_wifi();
void test_debug_soft_ap_config();
void test_debug_platform_info();

#endif // TEST_MAIN_H

