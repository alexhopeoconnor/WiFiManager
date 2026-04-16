#include <unity.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiManagerHandlers.h>
#include <TemplateEngine.h>
#include "templates/RootSelector.h"

namespace {

const char kEmptyTemplateChunk[] PROGMEM = "";
const char kTestTitle[] PROGMEM = "WiFiManager";
const char kTestSubtitle[] PROGMEM = "Root Test";

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

void test_root_render_interleaved_context_isolation() {
    Serial.println("[TEST]   Testing root render interleaved context isolation...");

    PlaceholderRegistry registryA(8);
    PlaceholderRegistry registryB(8);

    registryA.registerProgmemData("%SCRIPTS%", kEmptyTemplateChunk);
    registryA.registerProgmemData("%STYLES%", kEmptyTemplateChunk);
    registryA.registerProgmemData("%PAGE_TITLE%", kTestTitle);
    registryA.registerProgmemData("%SUBTITLE%", kTestSubtitle);

    registryB.registerProgmemData("%SCRIPTS%", kEmptyTemplateChunk);
    registryB.registerProgmemData("%STYLES%", kEmptyTemplateChunk);
    registryB.registerProgmemData("%PAGE_TITLE%", kTestTitle);
    registryB.registerProgmemData("%SUBTITLE%", kTestSubtitle);

    String menuA = F("<div>Menu A</div>");
    String statusA = F("<div>Status A</div>");
    String menuB = F("<div>Menu B</div>");
    String statusB = F("<div>Status B</div>");

    DynamicTemplateDescriptor menuDescriptorA{};
    DynamicTemplateDescriptor statusDescriptorA{};
    DynamicTemplateDescriptor menuDescriptorB{};
    DynamicTemplateDescriptor statusDescriptorB{};

    configureDescriptor(menuDescriptorA, menuA);
    configureDescriptor(statusDescriptorA, statusA);
    configureDescriptor(menuDescriptorB, menuB);
    configureDescriptor(statusDescriptorB, statusB);

    TEST_ASSERT_TRUE_MESSAGE(registryA.registerDynamicTemplate("%MENU%", &menuDescriptorA),
                             "Registry A menu placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryA.registerDynamicTemplate("%STATUS%", &statusDescriptorA),
                             "Registry A status placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryB.registerDynamicTemplate("%MENU%", &menuDescriptorB),
                             "Registry B menu placeholder should register");
    TEST_ASSERT_TRUE_MESSAGE(registryB.registerDynamicTemplate("%STATUS%", &statusDescriptorB),
                             "Registry B status placeholder should register");

    TemplateContext contextA;
    TemplateContext contextB;
    contextA.setRegistry(&registryA);
    contextB.setRegistry(&registryB);

    TemplateRenderer::initializeContext(contextA, WM_ROOT_TEMPLATE);
    TemplateRenderer::initializeContext(contextB, WM_ROOT_TEMPLATE);

    String outputA;
    String outputB;
    renderContextsInterleaved(contextA, outputA, contextB, outputB);

    TEST_ASSERT_NOT_EQUAL(-1, outputA.indexOf("Menu A"));
    TEST_ASSERT_NOT_EQUAL(-1, outputA.indexOf("Status A"));
    TEST_ASSERT_EQUAL(-1, outputA.indexOf("Menu B"));
    TEST_ASSERT_EQUAL(-1, outputA.indexOf("Status B"));

    TEST_ASSERT_NOT_EQUAL(-1, outputB.indexOf("Menu B"));
    TEST_ASSERT_NOT_EQUAL(-1, outputB.indexOf("Status B"));
    TEST_ASSERT_EQUAL(-1, outputB.indexOf("Menu A"));
    TEST_ASSERT_EQUAL(-1, outputB.indexOf("Status A"));

    Serial.println("[TEST]   Root render interleaved context isolation test completed successfully");
}
