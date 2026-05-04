#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiManagerHandlers.h>
#include <TemplateEngine.h>
#include "templates/RootShell.h"

namespace {

const char kEmptyTemplateChunk[] PROGMEM = "";
const char kTestTitle[] PROGMEM = "WiFiManager";

const char* dynamicStringGetter(void* userData) {
    const auto* value = static_cast<const String*>(userData);
    return value ? value->c_str() : "";
}

size_t dynamicStringLengthGetter(const char* data, void* /*userData*/) {
    return data ? strlen(data) : 0;
}

void configureDescriptor(DynamicDataDescriptor& descriptor, String& value) {
    descriptor.getter = &dynamicStringGetter;
    descriptor.getLength = &dynamicStringLengthGetter;
    descriptor.userData = &value;
}

void renderContextsInterleaved(TemplateContext& first, String& firstOutput,
                               TemplateContext& second, String& secondOutput) {
    uint8_t firstBuffer[31];
    uint8_t secondBuffer[31];
    bool firstDone = false;
    bool secondDone = false;

    while (!firstDone || !secondDone) {
        if (!firstDone) {
            size_t written = TemplateRenderer::renderNextChunk(first, firstBuffer, sizeof(firstBuffer));
            if (written == 0) {
                firstDone = true;
            } else {
                firstOutput.concat(reinterpret_cast<const char*>(firstBuffer), written);
            }
        }

        if (!secondDone) {
            size_t written = TemplateRenderer::renderNextChunk(second, secondBuffer, sizeof(secondBuffer));
            if (written == 0) {
                secondDone = true;
            } else {
                secondOutput.concat(reinterpret_cast<const char*>(secondBuffer), written);
            }
        }
    }
}

} // namespace

void test_bootstrap_json_portal_feature_flags() {
    Serial.println("[TEST]   Testing bootstrap JSON portal feature flags...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);

    String idleBootstrap = handlers.buildPortalBootstrapJson();
    TEST_ASSERT_EQUAL(-1, idleBootstrap.indexOf(F("\"closeCaptive\":{\"visible\":true")));

    wm.wmTestSetPortalActive(true);
    wm.portalSetBehaviorCaptivePortalEnabled(true);

    String apBootstrap = handlers.buildPortalBootstrapJson();
    TEST_ASSERT_NOT_EQUAL(-1, apBootstrap.indexOf(F("\"closeCaptive\":{\"visible\":true")));
    TEST_ASSERT_NOT_EQUAL(-1, apBootstrap.indexOf(F("\"exitPortal\":{\"visible\":true")));

    wm.wmTestSetPortalActive(false);

    Serial.println("[TEST]   Bootstrap portal feature flags test completed successfully");
}

void test_bootstrap_json_contract_v2() {
    Serial.println("[TEST]   Testing bootstrap JSON v2 contract...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);

    wm.portalSetBrandTitle("Solar Battery Monitor Setup");
    wm.portalSetContextIdentityText("Solar Battery Monitor");
    wm.portalSetBrandHomeIntro("Connect this device to WiFi and finish setup.");
    wm.portalSetBrandLogoSvg("<svg viewBox='0 0 24 24'></svg>");
    wm.portalSetPageInfoVisible(true);
    wm.portalSetPageUpdateVisible(false);
    wm.portalSetPageSetupVisible(true);
    wm.portalSetActionEraseVisible(false);
    wm.portalSetActionBackVisible(true);
    wm.portalSetLayoutParamsLocation(PortalParamsLocation::SetupPage);
    wm.setConfigPortalTimeout(90);

    PortalHomeCard card;
    card.id = "solar";
    card.title = "Solar summary";
    card.kind = PortalHomeCardKind::KeyValue;
    card.items.push_back({"pv", "PV input", "420W"});
    wm.portalAddHomeCard(card);

    String j = handlers.buildPortalBootstrapJson();
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"contractVersion\":2")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"brand\":{")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"title\":\"Solar Battery Monitor Setup\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"homeIntro\":\"Connect this device to WiFi and finish setup.\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"logoSvg\":\"<svg viewBox='0 0 24 24'></svg>\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"context\":{")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"identityText\":\"Solar Battery Monitor\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"pages\":{")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"setup\":{\"visible\":true}")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"actions\":{")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"erase\":{\"visible\":false}")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"back\":{\"visible\":true}")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"layout\":{\"paramsLocation\":\"setup\"}")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"extraHomeCards\":[")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"id\":\"solar\"")));
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"kind\":\"kv\"")));

    wm.startWebPortal();
    j = handlers.buildPortalBootstrapJson();
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"portalActive\":true")));
    wm.stopWebPortal();

    wm.wmTestSetPortalActive(true);
    j = handlers.buildPortalBootstrapJson();
    TEST_ASSERT_NOT_EQUAL(-1, j.indexOf(F("\"portalTimeoutSecondsRemaining\":")));
    TEST_ASSERT_EQUAL(-1, j.indexOf(F("\"portalTimeoutSecondsRemaining\":0")));
    wm.wmTestSetPortalActive(false);

    Serial.println("[TEST]   Bootstrap JSON v2 contract test completed successfully");
}

void test_bootstrap_json_snapshot_consistency() {
    Serial.println("[TEST]   Testing bootstrap JSON snapshot consistency...");

    WiFiManager wm;
    WiFiManagerHandlers handlers(&wm);

    wm.setConfigPortalTimeout(5);
    wm.startConfigPortal("RootSnapshotAP");
    wm.process();
    delay(100);

    String snap1 = handlers.buildPortalBootstrapJson();
    String snap2 = handlers.buildPortalBootstrapJson();

    TEST_ASSERT_GREATER_THAN(32, snap1.length());
    TEST_ASSERT_EQUAL_STRING(snap1.c_str(), snap2.c_str());

    wm.stopConfigPortal();

    Serial.println("[TEST]   Bootstrap JSON snapshot consistency test completed successfully");
}

void test_root_render_interleaved_context_isolation() {
    Serial.println("[TEST]   Testing root render interleaved context isolation...");

    PlaceholderRegistry registryA(8);
    PlaceholderRegistry registryB(8);

    registryA.registerProgmemData("%STYLES%", kEmptyTemplateChunk);
    registryA.registerProgmemData("%PAGE_TITLE%", kTestTitle);
    registryA.registerProgmemData("%PORTAL_APPEND_JS%", kEmptyTemplateChunk);

    registryB.registerProgmemData("%STYLES%", kEmptyTemplateChunk);
    registryB.registerProgmemData("%PAGE_TITLE%", kTestTitle);
    registryB.registerProgmemData("%PORTAL_APPEND_JS%", kEmptyTemplateChunk);

    String bootstrapA = F("{\"ctx\":\"A\"}");
    String appJsA = F("// shell A");
    String bootstrapB = F("{\"ctx\":\"B\"}");
    String appJsB = F("// shell B");

    DynamicDataDescriptor bootstrapDescriptorA{};
    DynamicDataDescriptor appJsDescriptorA{};
    DynamicDataDescriptor bootstrapDescriptorB{};
    DynamicDataDescriptor appJsDescriptorB{};

    configureDescriptor(bootstrapDescriptorA, bootstrapA);
    configureDescriptor(appJsDescriptorA, appJsA);
    configureDescriptor(bootstrapDescriptorB, bootstrapB);
    configureDescriptor(appJsDescriptorB, appJsB);

    TEST_ASSERT_TRUE_MESSAGE(registryA.registerDynamicData("%BOOTSTRAP_JSON%", &bootstrapDescriptorA),
                             "Registry A bootstrap placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryA.registerDynamicData("%PORTAL_APP_JS%", &appJsDescriptorA),
                             "Registry A app JS placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryB.registerDynamicData("%BOOTSTRAP_JSON%", &bootstrapDescriptorB),
                             "Registry B bootstrap placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryB.registerDynamicData("%PORTAL_APP_JS%", &appJsDescriptorB),
                             "Registry B app JS placeholder should register");

    TemplateContext contextA;
    TemplateContext contextB;
    contextA.setRegistry(&registryA);
    contextB.setRegistry(&registryB);

    TemplateRenderer::initializeContext(contextA, WM_ROOT_SHELL_TEMPLATE);
    TemplateRenderer::initializeContext(contextB, WM_ROOT_SHELL_TEMPLATE);

    String outputA;
    String outputB;
    renderContextsInterleaved(contextA, outputA, contextB, outputB);

    TEST_ASSERT_NOT_EQUAL(-1, outputA.indexOf("{\"ctx\":\"A\"}"));
    TEST_ASSERT_NOT_EQUAL(-1, outputA.indexOf("// shell A"));
    TEST_ASSERT_EQUAL(-1, outputA.indexOf("{\"ctx\":\"B\"}"));
    TEST_ASSERT_EQUAL(-1, outputA.indexOf("// shell B"));

    TEST_ASSERT_NOT_EQUAL(-1, outputB.indexOf("{\"ctx\":\"B\"}"));
    TEST_ASSERT_NOT_EQUAL(-1, outputB.indexOf("// shell B"));
    TEST_ASSERT_EQUAL(-1, outputB.indexOf("{\"ctx\":\"A\"}"));
    TEST_ASSERT_EQUAL(-1, outputB.indexOf("// shell A"));

    Serial.println("[TEST]   Root render interleaved context isolation test completed successfully");
}
