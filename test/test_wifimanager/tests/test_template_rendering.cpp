#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "templates/Fragments.h"
#include "templates/Info.h"
#include "templates/Param.h"
#include "templates/PageShell.h"
#include "templates/Root.h"
#include "templates/WiFi.h"
#include "templates/Message.h"
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
const char kBodyClass[] PROGMEM = "wifi";
const char kPageScripts[] PROGMEM = "<script>console.log('wifi page');</script>";
const char kPageStyles[] PROGMEM = "<style>.wifi{color:#000;}</style>";
const char kPageContent[] PROGMEM = "<div>Body</div>";
const char kScanContent[] PROGMEM = "Scanning for networks...<br/><br/>";
const char kSsidPlaceholder[] PROGMEM = "Office";
const char kPasswordPlaceholder[] PROGMEM = "********";
const char kStaticFields[] PROGMEM = "<hr><br/><label for='ip'>Static IP</label>";
const char kParamSection[] PROGMEM = "<hr><br/><label for='param_0'>Token</label>";
const char kFormActions[] PROGMEM = "<br/><br/><button type='submit'>Save</button>";
const char kPageActions[] PROGMEM = "<br/><div class='c'><button id='refresh-btn' type='button' onclick='return refreshScan()'>Refresh</button></div><hr><br/><form action='/' method='get'><button>Back</button></form>";
const char kMessageActions[] PROGMEM = "<hr><br/><form action='/' method='get'><button>Back</button></form>";
const char kStatus[] PROGMEM = "<div class='msg'>Status</div>";
const char kMessageBody[] PROGMEM = "<div class='msg S'>Saved</div>";
const char kInfoSection[] PROGMEM = "<h3>About</h3><hr><dl><dt>Build date</dt><dd>today</dd></dl>";
const char kInfoFooter[] PROGMEM = "<hr><br/><form action='/update' method='get'><button>Update</button></form>";
const char kPageTitle[] PROGMEM = "WiFiManager";
const char kSubtitle[] PROGMEM = "Setup";
const char kMenu[] PROGMEM = "<form><button>Configure WiFi</button></form>";
const char kActionPrefix[] PROGMEM = "<hr><br/>";
const char kActionPath[] PROGMEM = "/update";
const char kActionMethod[] PROGMEM = "get";
const char kActionLabel[] PROGMEM = "Update";
const char kActionClassAttr[] PROGMEM = " class='D'";
const char kActionSuffix[] PROGMEM = "<br/>\n";
const char kButtonId[] PROGMEM = "refresh-btn";
const char kButtonType[] PROGMEM = "button";
const char kButtonOnClick[] PROGMEM = "return refreshScan()";
const char kButtonExtraAttrs[] PROGMEM = " data-skip-initial-scan='true'";
const char kInfoLabel[] PROGMEM = "SDK version";
const char kInfoValue[] PROGMEM = "3.1.2";
const char kSectionTitle[] PROGMEM = "Device";
const char kSectionRows[] PROGMEM = "<dt>Chip ID</dt><dd>abc</dd>";

} // namespace

void test_page_shell_supports_page_assets() {
    Serial.println("[TEST]   Testing page shell page-scoped assets...");

    PlaceholderRegistry registry(12);
    registry.registerProgmemData("%SCRIPTS%", kEmpty);
    registry.registerProgmemData("%PAGE_SCRIPTS%", kPageScripts);
    registry.registerProgmemData("%STYLES%", kEmpty);
    registry.registerProgmemData("%PAGE_STYLES%", kPageStyles);
    registry.registerProgmemData("%DOC_TITLE%", kDocTitle);
    registry.registerProgmemData("%BODY_CLASS%", kBodyClass);
    registry.registerProgmemData("%PAGE_CONTENT%", kPageContent);

    String output = renderTemplate(WM_PAGE_SHELL_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("wifi page"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf(".wifi{color:#000;}"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("<div>Body</div>"));

    Serial.println("[TEST]   Page shell page-scoped assets test completed successfully");
}

void test_wifi_template_renders_scan_container_and_form_fields() {
    Serial.println("[TEST]   Testing WiFi page template rendering...");

    PlaceholderRegistry registry(16);
    registry.registerProgmemData("%WIFI_SCAN_CONTENT%", kScanContent);
    registry.registerProgmemData("%WIFI_SSID_PLACEHOLDER%", kSsidPlaceholder);
    registry.registerProgmemData("%WIFI_PASSWORD_PLACEHOLDER%", kPasswordPlaceholder);
    registry.registerProgmemData("%WIFI_STATIC_FIELDS%", kStaticFields);
    registry.registerProgmemData("%WIFI_PARAM_SECTION%", kParamSection);
    registry.registerProgmemData("%WIFI_FORM_ACTIONS%", kFormActions);
    registry.registerProgmemData("%WIFI_PAGE_ACTIONS%", kPageActions);
    registry.registerProgmemData("%WIFI_STATUS%", kStatus);

    String output = renderTemplate(WM_WIFI_CONTENT_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("id='scan-results'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Scanning for networks..."));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("placeholder='Office'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("id='refresh-btn'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Token"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Save"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Back"));

    Serial.println("[TEST]   WiFi page template rendering test completed successfully");
}

void test_message_template_renders_body_and_actions() {
    Serial.println("[TEST]   Testing message template rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%MESSAGE_BODY%", kMessageBody);
    registry.registerProgmemData("%MESSAGE_ACTIONS%", kMessageActions);

    String output = renderTemplate(WM_MESSAGE_CONTENT_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Saved"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Back"));

    Serial.println("[TEST]   Message template rendering test completed successfully");
}

void test_param_template_renders_shared_actions() {
    Serial.println("[TEST]   Testing param page template rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%PARAM_FIELDS%", kParamSection);
    registry.registerProgmemData("%PARAM_FORM_ACTIONS%", kFormActions);
    registry.registerProgmemData("%PARAM_PAGE_ACTIONS%", kMessageActions);
    registry.registerProgmemData("%PARAM_STATUS%", kStatus);

    String output = renderTemplate(WM_PARAM_CONTENT_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Token"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Save"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Back"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Status"));

    Serial.println("[TEST]   Param page template rendering test completed successfully");
}

void test_info_template_renders_sections_and_footer() {
    Serial.println("[TEST]   Testing info page template rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%INFO_STATUS%", kStatus);
    registry.registerProgmemData("%INFO_DEVICE_SECTION%", kInfoSection);
    registry.registerProgmemData("%INFO_WIFI_SECTION%", kInfoSection);
    registry.registerProgmemData("%INFO_ABOUT_SECTION%", kInfoSection);
    registry.registerProgmemData("%INFO_FOOTER%", kInfoFooter);

    String output = renderTemplate(WM_INFO_CONTENT_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Status"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Build date"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Update"));

    Serial.println("[TEST]   Info page template rendering test completed successfully");
}

void test_fragment_action_form_renders_shared_button_markup() {
    Serial.println("[TEST]   Testing shared action fragment rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%ACTION_PREFIX%", kActionPrefix);
    registry.registerProgmemData("%ACTION%", kActionPath);
    registry.registerProgmemData("%METHOD%", kActionMethod);
    registry.registerProgmemData("%BUTTON_LABEL%", kActionLabel);
    registry.registerProgmemData("%BUTTON_CLASS_ATTR%", kActionClassAttr);
    registry.registerProgmemData("%ACTION_SUFFIX%", kActionSuffix);

    String output = renderTemplate(WM_ACTION_FORM_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("action='/update'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("class='D'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf(">Update</button>"));

    Serial.println("[TEST]   Shared action fragment rendering test completed successfully");
}

void test_fragment_info_row_and_section_render() {
    Serial.println("[TEST]   Testing shared info fragment rendering...");

    PlaceholderRegistry rowRegistry(4);
    rowRegistry.registerProgmemData("%INFO_LABEL%", kInfoLabel);
    rowRegistry.registerProgmemData("%INFO_VALUE%", kInfoValue);

    String rowOutput = renderTemplate(WM_INFO_ROW_TEMPLATE, rowRegistry);
    TEST_ASSERT_NOT_EQUAL(-1, rowOutput.indexOf("<dt>SDK version</dt><dd>3.1.2</dd>"));

    PlaceholderRegistry sectionRegistry(6);
    sectionRegistry.registerProgmemData("%SECTION_PREFIX%", kEmpty);
    sectionRegistry.registerProgmemData("%SECTION_TITLE%", kSectionTitle);
    sectionRegistry.registerProgmemData("%SECTION_ROWS%", kSectionRows);
    sectionRegistry.registerProgmemData("%SECTION_SUFFIX%", kActionSuffix);

    String sectionOutput = renderTemplate(WM_INFO_SECTION_TEMPLATE, sectionRegistry);
    TEST_ASSERT_NOT_EQUAL(-1, sectionOutput.indexOf("<h3>Device</h3>"));
    TEST_ASSERT_NOT_EQUAL(-1, sectionOutput.indexOf("<dt>Chip ID</dt><dd>abc</dd>"));

    Serial.println("[TEST]   Shared info fragment rendering test completed successfully");
}

void test_fragment_centered_button_supports_extra_attrs() {
    Serial.println("[TEST]   Testing centered button fragment extra attrs...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%BUTTON_ID%", kButtonId);
    registry.registerProgmemData("%BUTTON_TYPE%", kButtonType);
    registry.registerProgmemData("%BUTTON_ONCLICK%", kButtonOnClick);
    registry.registerProgmemData("%BUTTON_LABEL%", kActionLabel);
    registry.registerProgmemData("%BUTTON_EXTRA_ATTRS%", kButtonExtraAttrs);

    String output = renderTemplate(WM_CENTERED_BUTTON_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("id='refresh-btn'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("data-skip-initial-scan='true'"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf(">Update</button>"));

    Serial.println("[TEST]   Centered button fragment extra attrs test completed successfully");
}

void test_root_template_renders_core_placeholders() {
    Serial.println("[TEST]   Testing root template rendering...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%SCRIPTS%", kEmpty);
    registry.registerProgmemData("%STYLES%", kEmpty);
    registry.registerProgmemData("%PAGE_TITLE%", kPageTitle);
    registry.registerProgmemData("%SUBTITLE%", kSubtitle);
    registry.registerProgmemData("%MENU%", kMenu);
    registry.registerProgmemData("%STATUS%", kStatus);

    String output = renderTemplate(WM_ROOT_TEMPLATE, registry);

    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("WiFiManager"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Setup"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Configure WiFi"));
    TEST_ASSERT_NOT_EQUAL(-1, output.indexOf("Status"));

    Serial.println("[TEST]   Root template rendering test completed successfully");
}
