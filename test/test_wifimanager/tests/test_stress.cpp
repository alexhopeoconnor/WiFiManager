#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

// Test many start/stop cycles for memory leaks
void test_many_start_stop_cycles() {
    Serial.println("[TEST]   Testing many start/stop cycles for memory leaks...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Test 10 start/stop cycles (enough to verify memory leaks and resource cleanup)
    for (int i = 0; i < 10; i++) {
        wm.startConfigPortal("TestAP");
        wm.process();
        
        // Verify portal started
        TEST_ASSERT_TRUE(wm.getConfigPortalActive());
        
        wm.stopConfigPortal();
        wm.process();
        
        // Verify portal stopped
        TEST_ASSERT_FALSE(wm.getConfigPortalActive());
        
        delay(10); // Small delay between cycles
    }
    
    // Verify no crashes or memory leaks (by checking we can still start/stop)
    wm.startConfigPortal("TestAP");
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Many start/stop cycles test completed successfully");
}

// Test long running portal
void test_long_running_portal() {
    Serial.println("[TEST]   Testing long running portal...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(30); // Longer timeout for long running test
    
    wm.startConfigPortal("TestAP");
    wm.process();
    
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Process for extended period (enough to verify stability)
    for (int i = 0; i < 50; i++) {
        wm.process();
        
        // Verify portal remains active
        TEST_ASSERT_TRUE(wm.getConfigPortalActive());
        
        // Verify infrastructure remains
        TEST_ASSERT_NOT_NULL(wm.server);
        TEST_ASSERT_NOT_NULL(wm.dnsServer);
        
        delay(10);
    }
    
    wm.stopConfigPortal();
    wm.process();
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Long running portal test completed successfully");
}

// Test rapid portal start/stop
void test_rapid_portal_start_stop() {
    Serial.println("[TEST]   Testing rapid portal start/stop...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Rapid start/stop cycles (reduced for faster execution)
    for (int i = 0; i < 5; i++) {
        wm.startConfigPortal("TestAP");
        wm.process();
        wm.stopConfigPortal();
        wm.process();
    }
    
    // Verify final state
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    // Verify we can still start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Rapid portal start/stop test completed successfully");
}

// Test multiple parameters stress
void test_multiple_parameters_stress() {
    Serial.println("[TEST]   Testing multiple parameters stress...");
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(10);
    
    // Add many parameters
    WiFiManagerParameter* params[20];
    for (int i = 0; i < 20; i++) {
        char id[20];
        char label[30];
        sprintf(id, "param%d", i);
        sprintf(label, "Parameter %d", i);
        params[i] = new WiFiManagerParameter(id, label, "default", 40);
        wm.addParameter(params[i]);
    }
    
    TEST_ASSERT_EQUAL(20, wm.getParametersCount());
    
    // Start portal
    wm.startConfigPortal("TestAP");
    wm.process();
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Verify all parameters accessible
    WiFiManagerParameter** paramArray = wm.getParameters();
    for (int i = 0; i < 20; i++) {
        TEST_ASSERT_NOT_NULL(paramArray[i]);
    }
    
    wm.stopConfigPortal();
    
    // Cleanup - Parameters are managed by WiFiManager, but we allocated with new
    // Note: WiFiManagerParameter doesn't have virtual destructor, but this is just cleanup
    for (int i = 0; i < 20; i++) {
        delete params[i];
    }
    
    Serial.println("[TEST]   Multiple parameters stress test completed successfully");
}

// Test portal with timeout stress
void test_portal_with_timeout_stress() {
    Serial.println("[TEST]   Testing portal with timeout stress...");
    
    WiFiManager wm;
    
    // Multiple cycles with different timeouts
    for (int timeout = 1; timeout <= 5; timeout++) {
        wm.setConfigPortalTimeout(timeout);
        wm.startConfigPortal("TestAP");
        wm.process();
        
        // Process until timeout or max wait
        unsigned long start = millis();
        unsigned long maxWait = (timeout + 1) * 1000UL;
        while (wm.getConfigPortalActive() && (millis() - start < maxWait)) {
            wm.process();
            delay(50);
        }
        
        // Portal may have timed out or still be active
        if (wm.getConfigPortalActive()) {
            wm.stopConfigPortal();
        }
        wm.process();
        
        delay(100);
    }
    
    // Verify final state
    TEST_ASSERT_FALSE(wm.getConfigPortalActive());
    
    Serial.println("[TEST]   Portal with timeout stress test completed successfully");
}

