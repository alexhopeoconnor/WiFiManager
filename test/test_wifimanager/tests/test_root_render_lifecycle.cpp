#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiManagerHandlers.h>

void test_root_render_menu_state_transitions() {
    Serial.println("[TEST]   Testing root menu state transitions...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);

    String menuBefore;
    handlers.getMenuOut(&menuBefore);
    TEST_ASSERT_EQUAL(-1, menuBefore.indexOf("/close"));

    wm.setConfigPortalTimeout(5);
    wm.startConfigPortal("RootStateAP");
    wm.process();
    delay(100);

    String menuActive;
    handlers.getMenuOut(&menuActive);
    TEST_ASSERT_NOT_EQUAL(-1, menuActive.indexOf("/close"));

    wm.stopConfigPortal();
    delay(100);

    String menuAfter;
    handlers.getMenuOut(&menuAfter);
    TEST_ASSERT_EQUAL(-1, menuAfter.indexOf("/close"));

    Serial.println("[TEST]   Root menu state transitions test completed successfully");
}

void test_root_render_snapshot_consistency() {
    Serial.println("[TEST]   Testing root render snapshot consistency...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);

    wm.setConfigPortalTimeout(5);
    wm.startConfigPortal("RootSnapshotAP");
    wm.process();
    delay(100);

    String menuSnapshot1;
    String statusSnapshot1;
    handlers.getMenuOut(&menuSnapshot1);
    handlers.reportStatus(statusSnapshot1);

    String menuSnapshot2;
    String statusSnapshot2;
    handlers.getMenuOut(&menuSnapshot2);
    handlers.reportStatus(statusSnapshot2);

    TEST_ASSERT_GREATER_THAN(0, menuSnapshot1.length());
    TEST_ASSERT_GREATER_THAN(0, statusSnapshot1.length());
    TEST_ASSERT_EQUAL_STRING(menuSnapshot1.c_str(), menuSnapshot2.c_str());
    TEST_ASSERT_EQUAL_STRING(statusSnapshot1.c_str(), statusSnapshot2.c_str());

    wm.stopConfigPortal();

    Serial.println("[TEST]   Root render snapshot consistency test completed successfully");
}
