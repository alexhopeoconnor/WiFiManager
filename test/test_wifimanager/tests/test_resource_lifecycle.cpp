#include <Arduino.h>
#include <WiFiManager.h>
#include <unity.h>
#include <vector>

#include "../test_main.h"

namespace {

struct HeapSnapshot {
    uint32_t freeHeap;
    uint32_t largestBlock;
    uint8_t fragmentation;
};

HeapSnapshot captureHeap(const char* label) {
    HeapSnapshot snapshot{
        ESP.getFreeHeap(),
#if defined(ESP8266)
        ESP.getMaxFreeBlockSize(),
        ESP.getHeapFragmentation(),
#else
        ESP.getMaxAllocHeap(),
        0,
#endif
    };

    Serial.printf("[METRIC] WM_HEAP label=%s free=%lu largest=%lu fragmentation=%u\n",
                  label,
                  static_cast<unsigned long>(snapshot.freeHeap),
                  static_cast<unsigned long>(snapshot.largestBlock),
                  snapshot.fragmentation);
    return snapshot;
}

uint32_t allowedHeapDrift() {
#if defined(ESP8266)
    return 1024;
#else
    return 4096;
#endif
}

uint32_t allowedLargestBlockDrift() {
#if defined(ESP8266)
    return 1024;
#else
    return 4096;
#endif
}

void assertRecovered(const HeapSnapshot& settled, const HeapSnapshot& final) {
    const uint32_t freeFloor = settled.freeHeap > allowedHeapDrift()
        ? settled.freeHeap - allowedHeapDrift()
        : 0;
    const uint32_t blockFloor = settled.largestBlock > allowedLargestBlockDrift()
        ? settled.largestBlock - allowedLargestBlockDrift()
        : 0;

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(
        freeFloor, final.freeHeap,
        "Portal start/stop cycles retained too much heap after warm-up");
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(
        blockFloor, final.largestBlock,
        "Portal start/stop cycles degraded the largest contiguous allocation");
}

void startAndStopPortal(WiFiManager& wm, const char* ssid) {
    wm.startConfigPortal(ssid);
    wm.process();
    TEST_ASSERT_TRUE_MESSAGE(wm.getConfigPortalActive(), "Portal should start");
    TEST_ASSERT_NOT_NULL_MESSAGE(wm.getServer(), "Portal server should exist");
    TEST_ASSERT_NOT_NULL_MESSAGE(wm.getDNSServer(), "Portal DNS server should exist");

    wm.stopConfigPortal();
    wm.process();
    TEST_ASSERT_FALSE_MESSAGE(wm.getConfigPortalActive(), "Portal should stop");
    TEST_ASSERT_NULL_MESSAGE(wm.getServer(), "Portal server should be released");
    TEST_ASSERT_NULL_MESSAGE(wm.getDNSServer(), "Portal DNS server should be released");
}

}  // namespace

void test_portal_resource_recovery() {
    Serial.println("[TEST]   Measuring portal resource recovery...");

    WiFiManager wm;
    wm.setConfigPortalTimeout(20);

    // Warm the Wi-Fi core once before establishing the comparison point. The
    // SDK is allowed to retain its own one-time allocations; the test detects
    // repeated decline after that settled point instead.
    startAndStopPortal(wm, "WM-Memory-Warmup");
    delay(200);
    const HeapSnapshot settled = captureHeap("settled");

    for (uint8_t cycle = 0; cycle < 20; ++cycle) {
        const String ssid = String("WM-Memory-") + String(cycle);
        wm.startConfigPortal(ssid.c_str());
        wm.process();
        TEST_ASSERT_TRUE_MESSAGE(wm.getConfigPortalActive(), "Portal should start during cycle");
        TEST_ASSERT_NOT_NULL_MESSAGE(wm.getServer(), "Server should exist during cycle");
        TEST_ASSERT_NOT_NULL_MESSAGE(wm.getDNSServer(), "DNS should exist during cycle");
        if (cycle == 0) {
            captureHeap("portal-active");
        }

        wm.stopConfigPortal();
        wm.process();
        TEST_ASSERT_FALSE_MESSAGE(wm.getConfigPortalActive(), "Portal should stop during cycle");
        TEST_ASSERT_NULL_MESSAGE(wm.getServer(), "Server should be released during cycle");
        TEST_ASSERT_NULL_MESSAGE(wm.getDNSServer(), "DNS should be released during cycle");
        captureHeap("cycle-stopped");
    }

    delay(200);
    const HeapSnapshot final = captureHeap("final");
    assertRecovered(settled, final);

    Serial.println("[TEST]   Portal resource recovery test completed successfully");
}

void test_scan_result_storage_released_when_portal_closes() {
    Serial.println("[TEST]   Testing scan-result storage release on portal close...");

    WiFiManager wm;
    wm.setConfigPortalTimeout(20);
    wm.startConfigPortal("WM-Scan-Storage");
    wm.process();

#ifdef UNIT_TEST
    std::vector<WiFiManager::WiFiScanNetwork> results;
    for (uint8_t i = 0; i < 24; ++i) {
        results.push_back({String("Network-") + String(i), -30 - i, static_cast<uint8_t>(i % 2)});
    }
    wm.wmTestInjectScanResults(results);
    captureHeap("scan-cache-held");
    TEST_ASSERT_GREATER_THAN_UINT32(0, wm.getScanResults().capacity());
#endif

    wm.stopConfigPortal();
    wm.process();
    TEST_ASSERT_EQUAL_UINT32(0, wm.getScanResults().size());
    TEST_ASSERT_EQUAL_UINT32(0, wm.getScanResults().capacity());
    captureHeap("scan-storage-cleared");

    Serial.println("[TEST]   Scan-result storage release test completed successfully");
}

void test_real_async_scan_completes() {
    Serial.println("[TEST]   Testing real asynchronous Wi-Fi scan completion...");

    WiFiManager wm;
    wm.setConfigPortalTimeout(30);
    wm.startConfigPortal("WM-Real-Scan");
    wm.process();
    TEST_ASSERT_TRUE_MESSAGE(wm.getConfigPortalActive(), "Portal should be active for scanning");
    captureHeap("scan-portal-active");

    wm.requestAsyncScan(true);
    captureHeap("scan-requested");
    const uint32_t deadline = millis() + 25000UL;
    while (wm.isScanRunning() && millis() < deadline) {
        wm.process();
        delay(20);
    }

    const WiFiManager::WiFiScanRuntimeState scan = wm.getScanSnapshot();
    Serial.printf("[METRIC] WM_SCAN state=%u result=%d count=%d elapsed=%lu\n",
                  static_cast<unsigned>(scan.state),
                  scan.lastScanResult,
                  static_cast<int>(wm.getScanResults().size()),
                  static_cast<unsigned long>(millis() - scan.startedAt));
    captureHeap("scan-complete-cache");

    const bool stillRunning = wm.isScanRunning();
    const WiFiManager::wm_scan_state_t state = wm.getScanState();
    wm.stopConfigPortal();

    captureHeap("scan-storage-cleared");
    TEST_ASSERT_FALSE_MESSAGE(stillRunning, "Async scan exceeded its completion deadline");
    TEST_ASSERT_EQUAL_MESSAGE(WiFiManager::WM_SCAN_COMPLETE, state,
                              "Real scan must complete rather than enter failed/timeout state");

    Serial.println("[TEST]   Real asynchronous Wi-Fi scan test completed successfully");
}
