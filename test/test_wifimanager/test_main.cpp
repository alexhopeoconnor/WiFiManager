#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "test_main.h"

// Test case array
TestCase tests[] = {
    TEST_ENTRY(test_basic_wifimanager_instantiation),
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

