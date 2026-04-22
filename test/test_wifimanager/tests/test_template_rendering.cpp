#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "templates/RootShell.h"
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

} // namespace

void test_shell_template_renders_core_placeholders() {
    Serial.println("[TEST]   Testing portal shell template rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%SCRIPTS%", kEmpty);
    registry.registerProgmemData("%STYLES%", kEmpty);
    registry.registerProgmemData("%PAGE_TITLE%", kDocTitle);
    registry.registerProgmemData("%BOOTSTRAP_JSON%", kBootstrapJson);
    registry.registerProgmemData("%PORTAL_APP_JS%", kPortalAppJs);

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
