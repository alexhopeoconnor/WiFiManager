#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test custom parameters
void test_create_parameter() {
    Serial.println("[TEST]   Testing WiFiManagerParameter creation...");
    
    // Create parameter with basic constructor
    WiFiManagerParameter param("test_id", "Test Label", "default_value", 40);
    
    // Verify basic getters work
    TEST_ASSERT_EQUAL_STRING("test_id", param.getID());
    TEST_ASSERT_EQUAL_STRING("Test Label", param.getLabel());
    TEST_ASSERT_EQUAL_STRING("default_value", param.getValue());
    TEST_ASSERT_EQUAL(40, param.getValueLength());
    
    Serial.println("[TEST]   WiFiManagerParameter creation test completed successfully");
}

void test_add_parameter() {
    Serial.println("[TEST]   Testing addParameter()...");
    
    WiFiManager wm;
    
    WiFiManagerParameter param1("param1", "Parameter 1", "value1", 40);
    WiFiManagerParameter param2("param2", "Parameter 2", "value2", 40);
    
    // Add parameters
    bool result1 = wm.addParameter(&param1);
    bool result2 = wm.addParameter(&param2);
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    
    // Verify parameters are stored
    TEST_ASSERT_EQUAL(2, wm.getParametersCount());
    
    Serial.println("[TEST]   addParameter() test completed successfully");
}

void test_get_parameters() {
    Serial.println("[TEST]   Testing getParameters() and getParametersCount()...");
    
    WiFiManager wm;
    
    WiFiManagerParameter param1("param1", "Parameter 1", "value1", 40);
    WiFiManagerParameter param2("param2", "Parameter 2", "value2", 40);
    WiFiManagerParameter param3("param3", "Parameter 3", "value3", 40);
    
    wm.addParameter(&param1);
    wm.addParameter(&param2);
    wm.addParameter(&param3);
    
    // Get count
    int count = wm.getParametersCount();
    TEST_ASSERT_EQUAL(3, count);
    
    // Get parameters array
    WiFiManagerParameter** params = wm.getParameters();
    TEST_ASSERT_NOT_NULL(params);
    
    // Verify we can access parameters
    TEST_ASSERT_EQUAL_STRING("param1", params[0]->getID());
    TEST_ASSERT_EQUAL_STRING("param2", params[1]->getID());
    TEST_ASSERT_EQUAL_STRING("param3", params[2]->getID());
    
    Serial.println("[TEST]   getParameters() test completed successfully");
}

void test_parameter_with_custom_html() {
    Serial.println("[TEST]   Testing WiFiManagerParameter with custom HTML...");
    
    // Create parameter with custom HTML
    const char* custom_html = "<input type='text' name='custom' value='test'>";
    WiFiManagerParameter param(custom_html);
    
    // Verify custom HTML is accessible
    const char* html = param.getCustomHTML();
    TEST_ASSERT_NOT_NULL(html);
    
    Serial.println("[TEST]   Parameter with custom HTML test completed successfully");
}

void test_parameter_value_length() {
    Serial.println("[TEST]   Testing WiFiManagerParameter value length...");
    
    WiFiManagerParameter param("test_id", "Test Label", "default", 10);
    
    TEST_ASSERT_EQUAL(10, param.getValueLength());
    
    // Try to set value within length
    param.setValue("short", 10);
    TEST_ASSERT_EQUAL_STRING("short", param.getValue());
    
    Serial.println("[TEST]   Parameter value length test completed successfully");
}

void test_parameter_label_placement() {
    Serial.println("[TEST]   Testing WiFiManagerParameter label placement...");
    
    WiFiManagerParameter param("test_id", "Test Label", "default", 40, "", WFM_LABEL_BEFORE);
    
    TEST_ASSERT_EQUAL(WFM_LABEL_BEFORE, param.getLabelPlacement());
    
    WiFiManagerParameter param2("test_id2", "Test Label 2", "default", 40, "", WFM_LABEL_AFTER);
    
    TEST_ASSERT_EQUAL(WFM_LABEL_AFTER, param2.getLabelPlacement());
    
    Serial.println("[TEST]   Parameter label placement test completed successfully");
}

void test_multiple_parameters() {
    Serial.println("[TEST]   Testing multiple parameters...");
    
    WiFiManager wm;
    
    // Add several parameters
    WiFiManagerParameter p1("mqtt_server", "MQTT Server", "192.168.1.1", 40);
    WiFiManagerParameter p2("mqtt_port", "MQTT Port", "1883", 6);
    WiFiManagerParameter p3("api_key", "API Key", "default_key", 32);
    
    wm.addParameter(&p1);
    wm.addParameter(&p2);
    wm.addParameter(&p3);
    
    TEST_ASSERT_EQUAL(3, wm.getParametersCount());
    
    WiFiManagerParameter** params = wm.getParameters();
    
    // Verify all parameters are accessible
    TEST_ASSERT_EQUAL_STRING("mqtt_server", params[0]->getID());
    TEST_ASSERT_EQUAL_STRING("mqtt_port", params[1]->getID());
    TEST_ASSERT_EQUAL_STRING("api_key", params[2]->getID());
    
    Serial.println("[TEST]   Multiple parameters test completed successfully");
}

void test_parameter_placeholder() {
    Serial.println("[TEST]   Testing WiFiManagerParameter placeholder (deprecated)...");
    
    WiFiManagerParameter param("test_id", "Test Label", "default", 40);
    
    // getPlaceholder() is deprecated but should still work
    const char* placeholder = param.getPlaceholder();
    // May return label or empty, just verify no crash
    TEST_ASSERT_NOT_NULL(placeholder);
    
    Serial.println("[TEST]   Parameter placeholder test completed successfully");
}

