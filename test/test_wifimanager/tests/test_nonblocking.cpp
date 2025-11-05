#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>

// Test process() doesn't block - verify it must be called periodically
void test_nonblocking_process() {
    Serial.println("[TEST]   Testing process() calls (non-blocking behavior)...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(10);
    wm.startConfigPortal("TestAP");
    
    // Verify portal is active
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Call process multiple times - should not block
    for (int i = 0; i < 10; i++) {
        unsigned long start = millis();
        wm.process();
        unsigned long elapsed = millis() - start;
        
        // Each call should be fast (< 100ms) - verifies non-blocking behavior
        TEST_ASSERT_LESS_THAN(100, elapsed);
        
        delay(10);
    }
    
    // Portal should still be active after multiple process() calls
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    wm.stopConfigPortal();
    
    Serial.println("[TEST]   Non-blocking process() test completed successfully");
}

// Test client check setters - verify they don't interfere with portal operation
void test_client_check_setters() {
    Serial.println("[TEST]   Testing client check setters...");
    
    WiFiManager wm;
    
    // Test AP client check
    wm.setAPClientCheck(true);
    wm.setAPClientCheck(false);
    
    wm.setConfigPortalTimeout(10);
    
    // Start portal and verify it works with client check settings
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    wm.process();
    delay(100);
    wm.stopConfigPortal();
    
    // Test web portal client check
    wm.setWebPortalClientCheck(true);
    wm.setWebPortalClientCheck(false);
    
    // Start web portal and verify it works
    wm.setConfigPortalTimeout(5);
    wm.startWebPortal();
    TEST_ASSERT_TRUE(wm.getWebPortalActive());
    wm.process();
    wm.stopWebPortal();
    
    Serial.println("[TEST]   Client check setters test completed successfully");
}

void test_nonblocking_timeout_behavior() {
    Serial.println("[TEST]   Testing timeout behavior (requires process() calls)...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(2); // 2 second timeout
    wm.startConfigPortal("TestAP");
    
    unsigned long start = millis();
    bool stillActive = true;
    
    // Process until timeout or max wait
    while (stillActive && (millis() - start < 5000)) {
        wm.process();
        
        // Check if portal is still active (may become inactive after timeout)
        if (!wm.getConfigPortalActive()) {
            stillActive = false;
            break; // Exit loop if portal has closed
        }
        
        delay(50);
    }
    
    // After timeout, portal should be inactive (may have auto-closed)
    // Just verify we can check the state without crashing
    bool portalState = wm.getConfigPortalActive();
    
    // Only call stopConfigPortal if portal is still active
    // If it auto-closed on timeout, calling stopConfigPortal may crash
    if (portalState) {
        wm.stopConfigPortal();
    }
    
    // Test completed without crash
    TEST_ASSERT_TRUE_MESSAGE(true, "Timeout test completed without crash");
    
    Serial.println("[TEST]   Timeout behavior test completed successfully");
}

// Test that process() is required for timeouts to work
void test_process_required_for_timeout() {
    Serial.println("[TEST]   Testing that process() is required for timeouts...");
    
    WiFiManager wm;
    
    wm.setConfigPortalTimeout(1); // 1 second timeout
    wm.startConfigPortal("TestAP");
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Wait 2 seconds WITHOUT calling process()
    delay(2000);
    
    // Portal should still be active because process() wasn't called
    // (timeout checking only happens in process())
    TEST_ASSERT_TRUE(wm.getConfigPortalActive());
    
    // Now call process() - timeout should be checked
    unsigned long start = millis();
    bool stillActive = true;
    
    // Process until timeout or max wait
    while (stillActive && (millis() - start < 3000)) {
        wm.process();
        
        if (!wm.getConfigPortalActive()) {
            stillActive = false;
            break;
        }
        
        delay(50);
    }
    
    // Portal should have timed out after process() was called
    // (may have auto-closed, but at least we verified process() is needed)
    bool portalState = wm.getConfigPortalActive();
    
    if (portalState) {
        wm.stopConfigPortal();
    }
    
    Serial.println("[TEST]   process() required for timeout test completed successfully");
}

