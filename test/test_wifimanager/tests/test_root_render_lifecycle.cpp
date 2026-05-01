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

void configureDescriptor(DynamicTemplateDescriptor& descriptor, String& value) {
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
    TEST_ASSERT_EQUAL(-1, idleBootstrap.indexOf(F("\"showCloseCaptive\":true")));

    wm.wmTestSetPortalActive(true);
    wm.setCaptivePortalEnable(true);

    String apBootstrap = handlers.buildPortalBootstrapJson();
    TEST_ASSERT_NOT_EQUAL(-1, apBootstrap.indexOf(F("\"showCloseCaptive\":true")));
    TEST_ASSERT_NOT_EQUAL(-1, apBootstrap.indexOf(F("\"showExitPortal\":true")));

    wm.wmTestSetPortalActive(false);

    Serial.println("[TEST]   Bootstrap portal feature flags test completed successfully");
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

    registryB.registerProgmemData("%STYLES%", kEmptyTemplateChunk);
    registryB.registerProgmemData("%PAGE_TITLE%", kTestTitle);

    String bootstrapA = F("{\"ctx\":\"A\"}");
    String appJsA = F("// shell A");
    String bootstrapB = F("{\"ctx\":\"B\"}");
    String appJsB = F("// shell B");

    DynamicTemplateDescriptor bootstrapDescriptorA{};
    DynamicTemplateDescriptor appJsDescriptorA{};
    DynamicTemplateDescriptor bootstrapDescriptorB{};
    DynamicTemplateDescriptor appJsDescriptorB{};

    configureDescriptor(bootstrapDescriptorA, bootstrapA);
    configureDescriptor(appJsDescriptorA, appJsA);
    configureDescriptor(bootstrapDescriptorB, bootstrapB);
    configureDescriptor(appJsDescriptorB, appJsB);

    TEST_ASSERT_TRUE_MESSAGE(registryA.registerDynamicTemplate("%BOOTSTRAP_JSON%", &bootstrapDescriptorA),
                             "Registry A bootstrap placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryA.registerDynamicTemplate("%PORTAL_APP_JS%", &appJsDescriptorA),
                             "Registry A app JS placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryB.registerDynamicTemplate("%BOOTSTRAP_JSON%", &bootstrapDescriptorB),
                             "Registry B bootstrap placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryB.registerDynamicTemplate("%PORTAL_APP_JS%", &appJsDescriptorB),
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
