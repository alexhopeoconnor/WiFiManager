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
const char kPortalTheme[] PROGMEM = "";

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
    registry.registerProgmemData("%PORTAL_THEME%", kPortalTheme);

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

void test_shell_template_renders_dynamic_theme_with_percent_values() {
    Serial.println("[TEST]   Testing portal shell dynamic theme placeholder with percent values...");

    PlaceholderRegistry registry(8);
    String theme = F("<style>:root{--wm-bg:rgb(20,50,30);--wm-width:100%}</style>");
    String title = F("Config ESP");
    String bootstrap = F("{\"title\":\"Test\"}");
    DynamicDataDescriptor themeDescriptor{};
    DynamicDataDescriptor titleDescriptor{};
    DynamicDataDescriptor bootstrapDescriptor{};

    configureDescriptor(themeDescriptor, theme);
    configureDescriptor(titleDescriptor, title);
    configureDescriptor(bootstrapDescriptor, bootstrap);

    TEST_ASSERT_TRUE(registry.registerProgmemData("%STYLES%", kEmpty));
    TEST_ASSERT_TRUE(registry.registerDynamicData("%PORTAL_THEME%", &themeDescriptor));
    TEST_ASSERT_TRUE(registry.registerDynamicData("%PAGE_TITLE%", &titleDescriptor));
    TEST_ASSERT_TRUE(registry.registerDynamicData("%BOOTSTRAP_JSON%", &bootstrapDescriptor));
    TEST_ASSERT_TRUE(registry.registerProgmemData("%PORTAL_APP_JS%", kPortalAppJs));

    String output = renderTemplate(WM_ROOT_SHELL_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<style>:root{--wm-bg:rgb(20,50,30);--wm-width:100%}</style>"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<title>Config ESP</title>"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("{\"title\":\"Test\"}"));

    Serial.println("[TEST]   Dynamic theme placeholder percent rendering test completed successfully");
}
