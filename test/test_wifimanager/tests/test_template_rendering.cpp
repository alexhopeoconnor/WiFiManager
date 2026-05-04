#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "templates/RootShell.h"
#include "templates/PortalAppJS.h"
#include "../test_main.h"

namespace {

String renderTemplate(const char* templateData, PlaceholderRegistry& registry) {
    TemplateContext context;
    context.setRegistry(&registry);
    TemplateRenderer::initializeContext(context, templateData);

    uint8_t buffer[128];
    String output;
    while (true) {
        size_t written = TemplateRenderer::renderNextChunk(context, buffer, sizeof(buffer));
        if (written == 0) {
            break;
        }
        output.concat(reinterpret_cast<const char*>(buffer), written);
    }
    return output;
}

const char kEmpty[] PROGMEM = "";
const char kDocTitle[] PROGMEM = "Config ESP";
const char kBootstrapJson[] PROGMEM = "{\"title\":\"Test\"}";
const char kPortalAppJs[] PROGMEM = "console.log('portal');";
const char kPortalAppendJs[] PROGMEM = "";

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

} // namespace

void test_shell_template_renders_core_placeholders() {
    Serial.println("[TEST]   Testing portal shell template rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%STYLES%", kEmpty);
    registry.registerProgmemData("%PAGE_TITLE%", kDocTitle);
    registry.registerProgmemData("%BOOTSTRAP_JSON%", kBootstrapJson);
    registry.registerProgmemData("%PORTAL_APP_JS%", kPortalAppJs);
    registry.registerProgmemData("%PORTAL_APPEND_JS%", kPortalAppendJs);

    String output = renderTemplate(WM_ROOT_SHELL_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<title>Config ESP</title>"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<div id='app'></div>"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("id='wm-toast'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("id='wm-dialog'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("id='wm-bootstrap'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("{\"title\":\"Test\"}"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("console.log('portal')"));

    Serial.println("[TEST]   Portal shell template rendering test completed successfully");
}

void test_shell_template_renders_dynamic_styles_with_percent_values() {
    Serial.println("[TEST]   Testing portal shell dynamic styles placeholder with percent values...");

    PlaceholderRegistry registry(8);
    String styles = F("<style>body{width:100%;max-width:92%}</style>");
    String title = F("Config ESP");
    String bootstrap = F("{\"title\":\"Test\"}");
    String appendJs = F("");
    DynamicDataDescriptor stylesDescriptor{};
    DynamicDataDescriptor titleDescriptor{};
    DynamicDataDescriptor bootstrapDescriptor{};
    DynamicDataDescriptor appendJsDescriptor{};

    configureDescriptor(stylesDescriptor, styles);
    configureDescriptor(titleDescriptor, title);
    configureDescriptor(bootstrapDescriptor, bootstrap);
    configureDescriptor(appendJsDescriptor, appendJs);

    TEST_ASSERT_TRUE(registry.registerDynamicData("%STYLES%", &stylesDescriptor));
    TEST_ASSERT_TRUE(registry.registerDynamicData("%PAGE_TITLE%", &titleDescriptor));
    TEST_ASSERT_TRUE(registry.registerDynamicData("%BOOTSTRAP_JSON%", &bootstrapDescriptor));
    TEST_ASSERT_TRUE(registry.registerProgmemData("%PORTAL_APP_JS%", kPortalAppJs));
    TEST_ASSERT_TRUE(registry.registerDynamicData("%PORTAL_APPEND_JS%", &appendJsDescriptor));

    String output = renderTemplate(WM_ROOT_SHELL_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<style>body{width:100%;max-width:92%}</style>"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<title>Config ESP</title>"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("{\"title\":\"Test\"}"));

    Serial.println("[TEST]   Dynamic styles placeholder percent rendering test completed successfully");
}
