#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include "../test_main.h"

void test_wifi_scan_initiates() {
    Serial.println("[TEST]   Testing async scan request queues...");

    WiFiManager wm;
    wm.requestAsyncScan(true);

    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_QUEUED, wm.getScanState());
    TEST_ASSERT_TRUE_MESSAGE(wm.getScanSnapshot().schedulePending,
                             "Explicit refresh should mark scan scheduling as pending");
    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_SCHEDULE_USER_REFRESH,
                      wm.getScanSnapshot().scheduledReason);
    TEST_ASSERT_FALSE_MESSAGE(wm.hasValidScanResults(),
                              "Queueing a scan should invalidate previous cached results");

    Serial.println("[TEST]   Async scan request queues test completed successfully");
}

void test_async_scan_behavior() {
    Serial.println("[TEST]   Testing repeated scan requests coalesce...");

    WiFiManager wm;
    wm.requestAsyncScan(true);
    wm.requestAsyncScan(true);
    wm.requestAsyncScan(true);

    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_QUEUED, wm.getScanState());
    TEST_ASSERT_TRUE_MESSAGE(wm.getScanSnapshot().schedulePending,
                             "Repeated refresh clicks should still leave one pending schedule");
    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_SCHEDULE_USER_REFRESH,
                      wm.getScanSnapshot().scheduledReason);
    TEST_ASSERT_FALSE_MESSAGE(wm.hasValidScanResults(),
                              "Repeated refresh requests should keep a single queued scan lifecycle");

    Serial.println("[TEST]   Repeated scan requests coalesce test completed successfully");
}

void test_scan_status_checking() {
    Serial.println("[TEST]   Testing cached scan snapshot state...");

    WiFiManager wm;

#ifdef UNIT_TEST
    std::vector<WiFiManager::WiFiScanNetwork> results = {
        { "Office", -48, 1 },
        { "Guest", -70, 0 }
    };
    wm.wmTestInjectScanResults(results);

    TEST_ASSERT_TRUE_MESSAGE(wm.hasValidScanResults(),
                             "Injected scan snapshot should be marked valid");
    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_COMPLETE, wm.getScanState());
    TEST_ASSERT_EQUAL(static_cast<size_t>(2), wm.getScanResults().size());
    TEST_ASSERT_EQUAL_STRING("Office", wm.getScanResults()[0].ssid.c_str());
#else
    TEST_IGNORE_MESSAGE("UNIT_TEST helpers unavailable");
#endif

    Serial.println("[TEST]   Cached scan snapshot state test completed successfully");
}

void test_scan_completion_wait() {
    Serial.println("[TEST]   Testing scan timeout transition...");

    WiFiManager wm;

#ifdef UNIT_TEST
    wm.wmTestSetPortalActive(true);
    wm.wmTestForceScanState(WiFiManager::WM_SCAN_RUNNING);
    wm.wmTestSetScanStartedAt(millis() - 20000);
    wm.wmTestSetScanTimeoutMs(1000);
    wm.process();

    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_TIMEOUT, wm.getScanState());
    TEST_ASSERT_FALSE_MESSAGE(wm.hasValidScanResults(),
                              "Timed out scans should not leave cached results marked valid");
#else
    TEST_IGNORE_MESSAGE("UNIT_TEST helpers unavailable");
#endif

    Serial.println("[TEST]   Scan timeout transition test completed successfully");
}

void test_scan_cancels_when_connect_pending() {
    Serial.println("[TEST]   Testing scan cancellation during connect...");

    WiFiManager wm;

#ifdef UNIT_TEST
    wm.wmTestSetPortalActive(true);
    wm.wmTestForceScanState(WiFiManager::WM_SCAN_RUNNING);
    wm.wmTestSetScanStartedAt(millis());
    wm.wmTestSetConnectPending(true);
    wm.process();

    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_IDLE, wm.getScanState());
    TEST_ASSERT_FALSE_MESSAGE(wm.getScanSnapshot().schedulePending,
                              "Connect flow should cancel any in-flight async scan");
#else
    TEST_IGNORE_MESSAGE("UNIT_TEST helpers unavailable");
#endif

    Serial.println("[TEST]   Scan cancellation during connect test completed successfully");
}

void test_scan_cancels_when_lifecycle_blocked() {
    Serial.println("[TEST]   Testing scan cancellation during lifecycle block...");

    WiFiManager wm;

#ifdef UNIT_TEST
    wm.wmTestSetPortalActive(true);
    wm.requestAsyncScan(true);
    wm.wmTestSetScanLifecycleBlocked(true);
    wm.process();

    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_IDLE, wm.getScanState());
    TEST_ASSERT_FALSE_MESSAGE(wm.getScanSnapshot().schedulePending,
                              "Lifecycle blocking should leave scan engine idle");
#else
    TEST_IGNORE_MESSAGE("UNIT_TEST helpers unavailable");
#endif

    Serial.println("[TEST]   Scan cancellation during lifecycle block test completed successfully");
}

void test_scan_generation_invalidated_on_reset() {
    Serial.println("[TEST]   Testing scan generation invalidation on reset...");

    WiFiManager wm;

#ifdef UNIT_TEST
    wm.wmTestSetScanGenerations(4, 4, 4);
    wm.wmTestSetScanCompletionPending(3);
    wm.wmTestClearScanResults();

    TEST_ASSERT_EQUAL(WiFiManager::WM_SCAN_IDLE, wm.getScanState());
    TEST_ASSERT_TRUE_MESSAGE(wm.getScanSnapshot().generation > 4,
                             "Resetting scan state should invalidate older completion generations");
    TEST_ASSERT_FALSE_MESSAGE(wm.getScanSnapshot().completionPending,
                              "Resetting scan state should clear pending completion callbacks");
#else
    TEST_IGNORE_MESSAGE("UNIT_TEST helpers unavailable");
#endif

    Serial.println("[TEST]   Scan generation invalidation on reset test completed successfully");
}

