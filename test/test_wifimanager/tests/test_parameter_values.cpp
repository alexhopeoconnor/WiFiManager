#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiManagerParameter.h>
#include "../test_main.h"

// Test parameter value set directly
void test_parameter_value_set_directly() {
    Serial.println("[TEST]   Testing parameter value set directly...");
    
    WiFiManagerParameter param("server", "Server", "default", 40);
    
    // Set value directly
    param.setValue("newvalue", 40);
    
    // Verify value was set
    TEST_ASSERT_EQUAL_STRING("newvalue", param.getValue());
    
    Serial.println("[TEST]   Parameter value set directly test completed successfully");
}

// Test parameter value length validation
void test_parameter_value_length_validation() {
    Serial.println("[TEST]   Testing parameter value length validation...");
    
    WiFiManagerParameter param("server", "Server", "", 10);
    
    // Set value within length
    param.setValue("short", 10);
    TEST_ASSERT_EQUAL_STRING("short", param.getValue());
    
    // Set value at length limit
    param.setValue("1234567890", 10); // Exactly 10 chars
    String value = param.getValue();
    TEST_ASSERT_LESS_OR_EQUAL(10, value.length());
    
    Serial.println("[TEST]   Parameter value length validation test completed successfully");
}

// Test parameter value persistence
void test_parameter_value_persistence() {
    Serial.println("[TEST]   Testing parameter value persistence...");
    
    WiFiManager wm;
    
    WiFiManagerParameter param("server", "Server", "default", 40);
    wm.portalAddParameter(&param);
    
    // Set value
    param.setValue("persisted_value", 40);
    TEST_ASSERT_EQUAL_STRING("persisted_value", param.getValue());
    
    // Verify parameter is still accessible through WiFiManager
    TEST_ASSERT_EQUAL(1, wm.getParametersCount());
    
    WiFiManagerParameter** params = wm.getParameters();
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_STRING("persisted_value", params[0]->getValue());
    
    Serial.println("[TEST]   Parameter value persistence test completed successfully");
}

// Test parameter value update
void test_parameter_value_update() {
    Serial.println("[TEST]   Testing parameter value update...");
    
    WiFiManagerParameter param("server", "Server", "default", 40);
    
    // Set initial value
    param.setValue("initial", 40);
    TEST_ASSERT_EQUAL_STRING("initial", param.getValue());
    
    // Update value
    param.setValue("updated", 40);
    TEST_ASSERT_EQUAL_STRING("updated", param.getValue());
    
    Serial.println("[TEST]   Parameter value update test completed successfully");
}

// Test multiple parameters with different values
void test_multiple_parameters_different_values() {
    Serial.println("[TEST]   Testing multiple parameters with different values...");
    
    WiFiManager wm;
    
    WiFiManagerParameter p1("server", "Server", "192.168.1.1", 40);
    WiFiManagerParameter p2("port", "Port", "1883", 6);
    WiFiManagerParameter p3("key", "Key", "default_key", 32);
    
    wm.portalAddParameter(&p1);
    wm.portalAddParameter(&p2);
    wm.portalAddParameter(&p3);
    
    // Set different values
    p1.setValue("10.0.0.1", 40);
    p2.setValue("8883", 6);
    p3.setValue("new_key", 32);
    
    // Verify all values are correct
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", p1.getValue());
    TEST_ASSERT_EQUAL_STRING("8883", p2.getValue());
    TEST_ASSERT_EQUAL_STRING("new_key", p3.getValue());
    
    // Verify through WiFiManager
    WiFiManagerParameter** params = wm.getParameters();
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", params[0]->getValue());
    TEST_ASSERT_EQUAL_STRING("8883", params[1]->getValue());
    TEST_ASSERT_EQUAL_STRING("new_key", params[2]->getValue());
    
    Serial.println("[TEST]   Multiple parameters with different values test completed successfully");
}

