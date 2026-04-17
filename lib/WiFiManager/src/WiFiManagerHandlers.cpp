/**
 * WiFiManagerHandlers.cpp
 * 
 * HTTP request handlers and rendering logic implementation
 * 
 * @author tablatronix
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#include "WiFiManagerHandlers.h"
#include "WiFiManagerServer.h" // Need menu tokens and routes
#include "templates/HTML.h"
#include "templates/CSS.h"
#include "templates/JS.h"
#include "templates/WiFiPollingJS.h"
#include "templates/PageShell.h"
#include "templates/Fragments.h"
#include "templates/Info.h"
#include "templates/Message.h"
#include "templates/Param.h"
#include "templates/WiFi.h"
#include "templates/RootSelector.h"
#include <TemplateEngine.h>
#include <cstring>

#if defined(ESP8266) || defined(ESP32)

#ifndef WM_PAGE_RESERVE_BYTES
#define WM_PAGE_RESERVE_BYTES 8192
#endif

namespace {

const char kEmptyTemplateChunk[] PROGMEM = "";

inline void reservePage(String& page, size_t extraBytes = WM_PAGE_RESERVE_BYTES) {
  if (extraBytes == 0) return;
  const size_t targetLen = page.length() + extraBytes;
  (void)page.reserve(targetLen);
}

const char* dynamicStringGetter(void* userData) {
  const auto* value = static_cast<const String*>(userData);
  return value ? value->c_str() : "";
}

size_t dynamicStringLengthGetter(const char* data, void* /*userData*/) {
  return data ? strlen(data) : 0;
}

void configureDynamicStringDescriptor(DynamicTemplateDescriptor& descriptor, String& value) {
  descriptor.getter = &dynamicStringGetter;
  descriptor.getLength = &dynamicStringLengthGetter;
  descriptor.userData = &value;
}

template <typename BundleT>
AsyncWebServerResponse* beginTemplateResponse(AsyncWebServerRequest* request,
                                             const std::shared_ptr<BundleT>& bundle,
                                             const char* templateData) {
  bundle->context.setRegistry(&bundle->registry);
  TemplateRenderer::initializeContext(bundle->context, templateData);
  return request->beginChunkedResponse(String(FPSTR(HTTP_HEAD_CT)),
    [bundle](uint8_t *buffer, size_t maxLen, size_t /*index*/) -> size_t {
      return TemplateRenderer::renderNextChunk(bundle->context, buffer, maxLen);
    }
  );
}

void registerSharedShellPlaceholders(WiFiManagerServer* server, PlaceholderRegistry& registry) {
  if (server) {
    server->registerDefaultStyles(registry);
    server->registerDefaultScripts(registry);
    server->applyTemplateSetupCallback(registry);
    return;
  }

  registry.registerProgmemData("%STYLES%", CSS_STYLE);
  registry.registerProgmemData("%SCRIPTS%", JS_SCRIPT);
}

void registerShellTemplate(PlaceholderRegistry& registry,
                           DynamicTemplateDescriptor& docTitleDescriptor,
                           String& docTitle,
                           const char* bodyClass,
                           const char* pageContentTemplate,
                           const char* pageScripts = kEmptyTemplateChunk,
                           const char* pageStyles = kEmptyTemplateChunk) {
  configureDynamicStringDescriptor(docTitleDescriptor, docTitle);
  registry.registerDynamicTemplate("%DOC_TITLE%", &docTitleDescriptor);
  registry.registerProgmemData("%BODY_CLASS%", bodyClass);
  registry.registerProgmemData("%PAGE_SCRIPTS%", pageScripts);
  registry.registerProgmemData("%PAGE_STYLES%", pageStyles);
  registry.registerProgmemTemplate("%PAGE_CONTENT%", pageContentTemplate);
}

struct RootState {
  String menu;
  String status;
};

struct MessagePageState {
  String docTitle;
  const char* bodyClass;
  String messageBody;
  String actions;

  MessagePageState() : bodyClass(C_root) {}
};

struct InfoPageState {
  String docTitle;
  String status;
  String deviceSection;
  String wifiSection;
  String aboutSection;
  String footer;
};

struct WiFiPageState {
  String docTitle;
  String scanContent;
  String ssidPlaceholder;
  String passwordPlaceholder;
  String staticFields;
  String paramSection;
  String formActions;
  String pageActions;
  String status;
};

struct ParamPageState {
  String docTitle;
  String fields;
  String formActions;
  String pageActions;
  String status;
};

void escapePercentsForTemplate(String& value) {
  if (value.indexOf('%') < 0) return;

  String escaped;
  reservePage(escaped, value.length() + 32);
  for (size_t i = 0; i < value.length(); i++) {
    if (value[i] == '%') {
      escaped += F("&#37;");
    } else {
      escaped += value[i];
    }
  }
  value = escaped;
}

void registerDynamicStringPlaceholder(PlaceholderRegistry& registry,
                                      const char* placeholder,
                                      String& value,
                                      DynamicTemplateDescriptor& descriptor) {
  escapePercentsForTemplate(value);
  configureDynamicStringDescriptor(descriptor, value);
  registry.registerDynamicTemplate(placeholder, &descriptor);
}

String renderTemplateToString(const char* templateData, PlaceholderRegistry& registry) {
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

String renderActionForm(const String& action,
                        const String& method,
                        const String& label,
                        const String& buttonClassAttr = String(),
                        const String& prefix = String(),
                        const String& suffix = String()) {
  PlaceholderRegistry registry(6);
  DynamicTemplateDescriptor actionDescriptor;
  DynamicTemplateDescriptor methodDescriptor;
  DynamicTemplateDescriptor labelDescriptor;
  DynamicTemplateDescriptor classDescriptor;
  DynamicTemplateDescriptor prefixDescriptor;
  DynamicTemplateDescriptor suffixDescriptor;

  String actionValue = action;
  String methodValue = method;
  String labelValue = label;
  String classValue = buttonClassAttr;
  String prefixValue = prefix;
  String suffixValue = suffix;

  registerDynamicStringPlaceholder(registry, "%ACTION%", actionValue, actionDescriptor);
  registerDynamicStringPlaceholder(registry, "%METHOD%", methodValue, methodDescriptor);
  registerDynamicStringPlaceholder(registry, "%BUTTON_LABEL%", labelValue, labelDescriptor);
  registerDynamicStringPlaceholder(registry, "%BUTTON_CLASS_ATTR%", classValue, classDescriptor);
  registerDynamicStringPlaceholder(registry, "%ACTION_PREFIX%", prefixValue, prefixDescriptor);
  registerDynamicStringPlaceholder(registry, "%ACTION_SUFFIX%", suffixValue, suffixDescriptor);

  return renderTemplateToString(WM_ACTION_FORM_TEMPLATE, registry);
}

String renderSubmitButton(const String& label) {
  PlaceholderRegistry registry(1);
  DynamicTemplateDescriptor labelDescriptor;
  String labelValue = label;
  registerDynamicStringPlaceholder(registry, "%BUTTON_LABEL%", labelValue, labelDescriptor);
  return renderTemplateToString(WM_SUBMIT_BUTTON_TEMPLATE, registry);
}

String renderCenteredButton(const String& id,
                            const String& type,
                            const String& onClick,
                            const String& label,
                            const String& extraAttrs = String()) {
  PlaceholderRegistry registry(5);
  DynamicTemplateDescriptor idDescriptor;
  DynamicTemplateDescriptor typeDescriptor;
  DynamicTemplateDescriptor onClickDescriptor;
  DynamicTemplateDescriptor labelDescriptor;
  DynamicTemplateDescriptor extraAttrsDescriptor;

  String idValue = id;
  String typeValue = type;
  String onClickValue = onClick;
  String labelValue = label;
  String extraAttrsValue = extraAttrs;

  registerDynamicStringPlaceholder(registry, "%BUTTON_ID%", idValue, idDescriptor);
  registerDynamicStringPlaceholder(registry, "%BUTTON_TYPE%", typeValue, typeDescriptor);
  registerDynamicStringPlaceholder(registry, "%BUTTON_ONCLICK%", onClickValue, onClickDescriptor);
  registerDynamicStringPlaceholder(registry, "%BUTTON_LABEL%", labelValue, labelDescriptor);
  registerDynamicStringPlaceholder(registry, "%BUTTON_EXTRA_ATTRS%", extraAttrsValue, extraAttrsDescriptor);

  return renderTemplateToString(WM_CENTERED_BUTTON_TEMPLATE, registry);
}

String renderSectionBreak(const String& content) {
  if (content.length() == 0) {
    return String();
  }

  PlaceholderRegistry registry(1);
  DynamicTemplateDescriptor contentDescriptor;
  String contentValue = content;
  registerDynamicStringPlaceholder(registry, "%SECTION_CONTENT%", contentValue, contentDescriptor);
  return renderTemplateToString(WM_SECTION_BREAK_TEMPLATE, registry);
}

String renderScanMessage(const String& message) {
  PlaceholderRegistry registry(1);
  DynamicTemplateDescriptor messageDescriptor;
  String messageValue = message;
  registerDynamicStringPlaceholder(registry, "%SCAN_MESSAGE%", messageValue, messageDescriptor);
  return renderTemplateToString(WM_SCAN_MESSAGE_TEMPLATE, registry);
}

String renderScanRow(const String& ssidAttr,
                     const String& ssidText,
                     const String& qualityLabel,
                     const String& qualityIcon,
                     const String& lockClass,
                     const String& iconVisibilityClass,
                     const String& valueVisibilityClass,
                     const String& qualityValue) {
  PlaceholderRegistry registry(8);
  DynamicTemplateDescriptor ssidAttrDescriptor;
  DynamicTemplateDescriptor ssidTextDescriptor;
  DynamicTemplateDescriptor qualityLabelDescriptor;
  DynamicTemplateDescriptor qualityIconDescriptor;
  DynamicTemplateDescriptor lockClassDescriptor;
  DynamicTemplateDescriptor iconVisibilityDescriptor;
  DynamicTemplateDescriptor valueVisibilityDescriptor;
  DynamicTemplateDescriptor qualityValueDescriptor;

  String ssidAttrValue = ssidAttr;
  String ssidTextValue = ssidText;
  String qualityLabelValue = qualityLabel;
  String qualityIconValue = qualityIcon;
  String lockClassValue = lockClass;
  String iconVisibilityValue = iconVisibilityClass;
  String valueVisibilityValue = valueVisibilityClass;
  String qualityValueValue = qualityValue;

  registerDynamicStringPlaceholder(registry, "%SSID_ATTR%", ssidAttrValue, ssidAttrDescriptor);
  registerDynamicStringPlaceholder(registry, "%SSID_TEXT%", ssidTextValue, ssidTextDescriptor);
  registerDynamicStringPlaceholder(registry, "%QUALITY_LABEL%", qualityLabelValue, qualityLabelDescriptor);
  registerDynamicStringPlaceholder(registry, "%QUALITY_ICON%", qualityIconValue, qualityIconDescriptor);
  registerDynamicStringPlaceholder(registry, "%LOCK_CLASS%", lockClassValue, lockClassDescriptor);
  registerDynamicStringPlaceholder(registry, "%ICON_VISIBILITY_CLASS%", iconVisibilityValue, iconVisibilityDescriptor);
  registerDynamicStringPlaceholder(registry, "%VALUE_VISIBILITY_CLASS%", valueVisibilityValue, valueVisibilityDescriptor);
  registerDynamicStringPlaceholder(registry, "%QUALITY_VALUE%", qualityValueValue, qualityValueDescriptor);

  return renderTemplateToString(WM_SCAN_RESULT_ROW_TEMPLATE, registry);
}

String renderFieldTemplate(const char* templateData,
                           const String& id,
                           const String& label,
                           const String& maxLength,
                           const String& value,
                           const String& extraAttrs = String()) {
  PlaceholderRegistry registry(6);
  DynamicTemplateDescriptor idDescriptor;
  DynamicTemplateDescriptor nameDescriptor;
  DynamicTemplateDescriptor labelDescriptor;
  DynamicTemplateDescriptor maxLengthDescriptor;
  DynamicTemplateDescriptor valueDescriptor;
  DynamicTemplateDescriptor extraAttrsDescriptor;

  String idValue = id;
  String nameValue = id;
  String labelValue = label;
  String maxLengthValue = maxLength;
  String valueValue = value;
  String extraAttrsValue = extraAttrs;

  registerDynamicStringPlaceholder(registry, "%FIELD_ID%", idValue, idDescriptor);
  registerDynamicStringPlaceholder(registry, "%FIELD_NAME%", nameValue, nameDescriptor);
  registerDynamicStringPlaceholder(registry, "%FIELD_LABEL%", labelValue, labelDescriptor);
  registerDynamicStringPlaceholder(registry, "%FIELD_MAXLENGTH%", maxLengthValue, maxLengthDescriptor);
  registerDynamicStringPlaceholder(registry, "%FIELD_VALUE%", valueValue, valueDescriptor);
  registerDynamicStringPlaceholder(registry, "%FIELD_EXTRA_ATTRS%", extraAttrsValue, extraAttrsDescriptor);

  return renderTemplateToString(templateData, registry);
}

String renderInfoRow(const String& label, const String& value) {
  PlaceholderRegistry registry(2);
  DynamicTemplateDescriptor labelDescriptor;
  DynamicTemplateDescriptor valueDescriptor;
  String labelValue = label;
  String valueValue = value;
  registerDynamicStringPlaceholder(registry, "%INFO_LABEL%", labelValue, labelDescriptor);
  registerDynamicStringPlaceholder(registry, "%INFO_VALUE%", valueValue, valueDescriptor);
  return renderTemplateToString(WM_INFO_ROW_TEMPLATE, registry);
}

String renderInfoSection(const String& title,
                         const String& rows,
                         const String& prefix = String(),
                         const String& suffix = String()) {
  PlaceholderRegistry registry(4);
  DynamicTemplateDescriptor titleDescriptor;
  DynamicTemplateDescriptor rowsDescriptor;
  DynamicTemplateDescriptor prefixDescriptor;
  DynamicTemplateDescriptor suffixDescriptor;

  String titleValue = title;
  String rowsValue = rows;
  String prefixValue = prefix;
  String suffixValue = suffix;

  registerDynamicStringPlaceholder(registry, "%SECTION_TITLE%", titleValue, titleDescriptor);
  registerDynamicStringPlaceholder(registry, "%SECTION_ROWS%", rowsValue, rowsDescriptor);
  registerDynamicStringPlaceholder(registry, "%SECTION_PREFIX%", prefixValue, prefixDescriptor);
  registerDynamicStringPlaceholder(registry, "%SECTION_SUFFIX%", suffixValue, suffixDescriptor);

  return renderTemplateToString(WM_INFO_SECTION_TEMPLATE, registry);
}

String renderStatusMessage(const String& title,
                           const String& body,
                           const String& statusClassSuffix = String()) {
  PlaceholderRegistry registry(3);
  DynamicTemplateDescriptor titleDescriptor;
  DynamicTemplateDescriptor bodyDescriptor;
  DynamicTemplateDescriptor classDescriptor;

  String titleValue = title;
  String bodyValue = body;
  String classValue = statusClassSuffix;

  registerDynamicStringPlaceholder(registry, "%STATUS_TITLE%", titleValue, titleDescriptor);
  registerDynamicStringPlaceholder(registry, "%STATUS_BODY%", bodyValue, bodyDescriptor);
  registerDynamicStringPlaceholder(registry, "%STATUS_CLASS_SUFFIX%", classValue, classDescriptor);

  return renderTemplateToString(WM_STATUS_MESSAGE_TEMPLATE, registry);
}

void buildRootState(WiFiManagerHandlers* handlers, RootState& state) {
  reservePage(state.menu, 768);
  reservePage(state.status, 512);

  handlers->getMenuOut(&state.menu);
  handlers->reportStatus(state.status);

  // Dynamic templates are parsed as template content, so escape literal '%' values.
  escapePercentsForTemplate(state.menu);
  escapePercentsForTemplate(state.status);
}

String renderPageHeading(const String& title, const String& subtitle) {
  PlaceholderRegistry registry(2);
  DynamicTemplateDescriptor titleDescriptor;
  DynamicTemplateDescriptor subtitleDescriptor;
  String titleValue = title;
  String subtitleValue = subtitle;
  registerDynamicStringPlaceholder(registry, "%HEADER_TITLE%", titleValue, titleDescriptor);
  registerDynamicStringPlaceholder(registry, "%HEADER_SUBTITLE%", subtitleValue, subtitleDescriptor);
  return renderTemplateToString(WM_PAGE_HEADING_TEMPLATE, registry);
}

void buildMessagePageState(MessagePageState& state,
                           const String& title,
                           const char* bodyClass,
                           const String& messageBodyHtml,
                           const String& actionsHtml) {
  state.docTitle = title;
  state.bodyClass = bodyClass;
  state.messageBody = messageBodyHtml;
  state.actions = actionsHtml;
  escapePercentsForTemplate(state.messageBody);
  escapePercentsForTemplate(state.actions);
}

String buildUpdatePanelContent(const String& title, const String& subtitle) {
  String content;
  reservePage(content, 2048);
  content += renderPageHeading(title, subtitle);
  content += FPSTR(HTML_UPDATE);
  return content;
}

String buildUpdateResultContent(const String& title, const String& subtitle) {
  String content;
  reservePage(content, 2048);
  content += renderPageHeading(title, subtitle);

  if (Update.hasError()) {
    content += FPSTR(HTML_UPDATE_FAIL);
    #ifdef ESP32
    content += "OTA Error: " + String(Update.errorString());
    #else
    content += "OTA Error: " + String(Update.getError());
    #endif
  } else {
    content += FPSTR(HTML_UPDATE_SUCCESS);
  }

  return content;
}

void appendInfoEntries(WiFiManagerHandlers* handlers,
                       String& section,
                       const char* const* ids,
                       size_t count) {
  for (size_t i = 0; i < count; i++) {
    section += handlers->getInfoData(ids[i]);
  }
}

void buildInfoPageState(WiFiManagerHandlers* handlers,
                        InfoPageState& state,
                        bool showInfoUpdate,
                        bool showInfoErase,
                        bool showBack) {
  reservePage(state.status, 512);
  reservePage(state.deviceSection, 4096);
  reservePage(state.wifiSection, 4096);
  reservePage(state.aboutSection, 1536);
  reservePage(state.footer, 1024);

  state.docTitle = F("Info");
  handlers->reportStatus(state.status);

#ifdef ESP8266
  static const char* const deviceIds[] = {
    "uptime", "chipid", "fchipid", "idesize", "flashsize", "corever",
    "bootver", "cpufreq", "freeheap", "memsketch", "memsmeter", "lastreset"
  };
  static const char* const wifiIds[] = {
    "conx", "stassid", "staip", "stagw", "stasub", "dnss", "host",
    "stamac", "autoconx", "apssid", "apip", "apbssid", "apmac"
  };
#elif defined(ESP32)
  static const char* const deviceIds[] = {
    "uptime", "chipid", "chiprev", "idesize", "flashsize",
    "cpufreq", "freeheap", "memsketch", "memsmeter", "lastreset", "temp"
  };
  static const char* const wifiIds[] = {
    "conx", "stassid", "staip", "stagw", "stasub", "dnss", "host",
    "stamac", "apssid", "apip", "apmac", "aphost", "apbssid"
  };
#endif

  appendInfoEntries(handlers, state.deviceSection, deviceIds, sizeof(deviceIds) / sizeof(deviceIds[0]));
  appendInfoEntries(handlers, state.wifiSection, wifiIds, sizeof(wifiIds) / sizeof(wifiIds[0]));
  state.aboutSection += handlers->getInfoData("aboutver");
  state.aboutSection += handlers->getInfoData("aboutarduinover");
  state.aboutSection += handlers->getInfoData("aboutsdkver");
  state.aboutSection += handlers->getInfoData("aboutdate");

  if (showInfoUpdate) {
    state.footer += renderActionForm(F("/update"), F("get"), F("Update"), String(), F("<hr><br/>"), F("<br/>\n"));
  }
  if (showInfoErase) {
    state.footer += renderActionForm(F("/erase"), F("get"), F("Erase WiFi config"), F(" class='D'"), showInfoUpdate ? String() : String(F("<hr><br/>")), F("<br/>\n"));
  }
  if (showBack) {
    state.footer += renderActionForm(F("/"), F("get"), F("Back"), String(), (showInfoUpdate || showInfoErase) ? String() : String(F("<hr><br/>")));
  }
  state.footer += FPSTR(HTML_HELP);

  state.deviceSection = renderInfoSection(
#ifdef ESP32
    F("ESP32"),
#else
    F("ESP8266"),
#endif
    state.deviceSection
  );
  state.wifiSection = renderInfoSection(F("WiFi"), state.wifiSection, F("<br/>"));
  state.aboutSection = renderInfoSection(F("About"), state.aboutSection, F("<br/>"));

  escapePercentsForTemplate(state.status);
  escapePercentsForTemplate(state.deviceSection);
  escapePercentsForTemplate(state.wifiSection);
  escapePercentsForTemplate(state.aboutSection);
  escapePercentsForTemplate(state.footer);
}

void buildWiFiPageState(WiFiManagerHandlers* handlers,
                        WiFiPageState& state,
                        bool includeScanResults,
                        bool showBack,
                        const String& ssidPlaceholder,
                        const String& passwordPlaceholder,
                        bool paramsInWifi) {
  reservePage(state.scanContent, includeScanResults ? 4096 : 64);
  reservePage(state.staticFields, 2048);
  reservePage(state.paramSection, 4096);
  reservePage(state.formActions, 128);
  reservePage(state.pageActions, 512);
  reservePage(state.status, 512);

  state.docTitle = F("Config ESP");
  state.ssidPlaceholder = ssidPlaceholder;
  state.passwordPlaceholder = passwordPlaceholder;

  if (includeScanResults) {
    state.scanContent += handlers->getScanItemOut();
  }

  state.staticFields += handlers->getStaticOut();
  if (paramsInWifi) {
    state.paramSection += renderSectionBreak(handlers->getParamOut());
  }

  state.formActions += renderSubmitButton(F("Save"));
  state.pageActions += renderCenteredButton(
    F("refresh-btn"),
    F("button"),
    F("return refreshScan()"),
    F("Refresh"),
    includeScanResults ? String() : String(F(" data-skip-initial-scan='true'"))
  );
  if (showBack) state.pageActions += renderActionForm(F("/"), F("get"), F("Back"), String(), F("<hr><br/>"));

  handlers->reportStatus(state.status);

  escapePercentsForTemplate(state.scanContent);
  escapePercentsForTemplate(state.ssidPlaceholder);
  escapePercentsForTemplate(state.passwordPlaceholder);
  escapePercentsForTemplate(state.staticFields);
  escapePercentsForTemplate(state.paramSection);
  escapePercentsForTemplate(state.formActions);
  escapePercentsForTemplate(state.pageActions);
  escapePercentsForTemplate(state.status);
}

void buildParamPageState(WiFiManagerHandlers* handlers,
                         ParamPageState& state,
                         bool showBack) {
  reservePage(state.fields, 4096);
  reservePage(state.formActions, 128);
  reservePage(state.pageActions, 256);
  reservePage(state.status, 512);

  state.docTitle = F("Setup");
  state.fields += handlers->getParamOut();
  state.formActions += renderSubmitButton(F("Save"));

  if (showBack) state.pageActions += renderActionForm(F("/"), F("get"), F("Back"), String(), F("<hr><br/>"));

  handlers->reportStatus(state.status);

  escapePercentsForTemplate(state.fields);
  escapePercentsForTemplate(state.formActions);
  escapePercentsForTemplate(state.pageActions);
  escapePercentsForTemplate(state.status);
}

struct RootRenderBundle {
  RootState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor menuDescriptor;
  DynamicTemplateDescriptor statusDescriptor;

  RootRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY), menuDescriptor{}, statusDescriptor{} {}
};

struct MessageRenderBundle {
  MessagePageState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor docTitleDescriptor;
  DynamicTemplateDescriptor messageBodyDescriptor;
  DynamicTemplateDescriptor actionsDescriptor;

  MessageRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        messageBodyDescriptor{},
        actionsDescriptor{} {}
};

struct InfoRenderBundle {
  InfoPageState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor docTitleDescriptor;
  DynamicTemplateDescriptor statusDescriptor;
  DynamicTemplateDescriptor deviceDescriptor;
  DynamicTemplateDescriptor wifiDescriptor;
  DynamicTemplateDescriptor aboutDescriptor;
  DynamicTemplateDescriptor footerDescriptor;

  InfoRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        statusDescriptor{},
        deviceDescriptor{},
        wifiDescriptor{},
        aboutDescriptor{},
        footerDescriptor{} {}
};

struct WiFiRenderBundle {
  WiFiPageState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor docTitleDescriptor;
  DynamicTemplateDescriptor scanContentDescriptor;
  DynamicTemplateDescriptor ssidPlaceholderDescriptor;
  DynamicTemplateDescriptor passwordPlaceholderDescriptor;
  DynamicTemplateDescriptor staticFieldsDescriptor;
  DynamicTemplateDescriptor paramSectionDescriptor;
  DynamicTemplateDescriptor formActionsDescriptor;
  DynamicTemplateDescriptor pageActionsDescriptor;
  DynamicTemplateDescriptor statusDescriptor;

  WiFiRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        scanContentDescriptor{},
        ssidPlaceholderDescriptor{},
        passwordPlaceholderDescriptor{},
        staticFieldsDescriptor{},
        paramSectionDescriptor{},
        formActionsDescriptor{},
        pageActionsDescriptor{},
        statusDescriptor{} {}
};

struct ParamRenderBundle {
  ParamPageState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor docTitleDescriptor;
  DynamicTemplateDescriptor fieldsDescriptor;
  DynamicTemplateDescriptor formActionsDescriptor;
  DynamicTemplateDescriptor pageActionsDescriptor;
  DynamicTemplateDescriptor statusDescriptor;

  ParamRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        fieldsDescriptor{},
        formActionsDescriptor{},
        pageActionsDescriptor{},
        statusDescriptor{} {}
};

void sendMessageTemplateResponse(WiFiManagerServer* server,
                                 const std::shared_ptr<MessageRenderBundle>& bundle,
                                 const String& title,
                                 const char* bodyClass,
                                 const String& messageBody,
                                 const String& actions = String()) {
  buildMessagePageState(bundle->state, title, bodyClass, messageBody, actions);

  registerSharedShellPlaceholders(server, bundle->registry);
  registerShellTemplate(bundle->registry,
                        bundle->docTitleDescriptor,
                        bundle->state.docTitle,
                        bundle->state.bodyClass,
                        WM_MESSAGE_CONTENT_TEMPLATE);
  configureDynamicStringDescriptor(bundle->messageBodyDescriptor, bundle->state.messageBody);
  configureDynamicStringDescriptor(bundle->actionsDescriptor, bundle->state.actions);

  bundle->registry.registerDynamicTemplate("%MESSAGE_BODY%", &bundle->messageBodyDescriptor);
  bundle->registry.registerDynamicTemplate("%MESSAGE_ACTIONS%", &bundle->actionsDescriptor);
}

}  // namespace

WiFiManagerHandlers::WiFiManagerHandlers(WiFiManager* wm) : _wm(wm) {}

// Rendering Methods

String WiFiManagerHandlers::getMenuOut(){
  return getMenuOut(nullptr);
}

String WiFiManagerHandlers::getMenuOut(String* outOpt){
  String local;
  String &out = outOpt ? *outOpt : local;
  out += renderActionForm(F("/wifi"), F("get"), F("Configure WiFi"), String(), String(), F("<br/>\n"));
  
  // Show PARAM (Setup) when params are on their own page
  if(!_wm->_paramsInWifi && _wm->_paramsCount > 0){
    out += renderActionForm(F("/param"), F("get"), F("Setup"), String(), String(), F("<br/>\n"));
  }
  
  out += renderActionForm(F("/info"), F("get"), F("Info"), String(), String(), F("<br/>\n"));
  
  // When captive/config portal is active, offer Close (keeps portal running)
  if(_wm->configPortalActive){
    out += renderActionForm(F("/close"), F("get"), F("Close"), String(), String(), F("<br/>\n"));
  }
  
  out += renderActionForm(F("/exit"), F("get"), F("Exit"), String(), String(), F("<br/>\n"));
  out += F("<hr><br/>");
  out += renderActionForm(F("/update"), F("get"), F("Update"), String(), String(), F("<br/>\n"));
  return out;
}

void WiFiManagerHandlers::collectVisibleScanResults(std::vector<const WiFiManager::WiFiScanNetwork*>& networks) {
  networks.clear();
  networks.reserve(_wm->_scanResultsCache.size());

  for (const auto& network : _wm->_scanResultsCache) {
    if (network.ssid.length() == 0) {
      continue;
    }

    int rssiperc = _wm->getRSSIasQuality(network.rssi);
    if (_wm->_minimumQuality != -1 && _wm->_minimumQuality >= rssiperc) {
      continue;
    }

    networks.push_back(&network);
  }

  const size_t n = networks.size();
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      if (networks[j]->rssi > networks[i]->rssi) {
        std::swap(networks[i], networks[j]);
      }
    }
  }

  if (_wm->_removeDuplicateAPs) {
    std::vector<const WiFiManager::WiFiScanNetwork*> deduped;
    deduped.reserve(networks.size());
    for (const auto* network : networks) {
      bool duplicate = false;
      for (const auto* existing : deduped) {
        if (existing->ssid == network->ssid) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate) {
        deduped.push_back(network);
      }
    }
    networks.swap(deduped);
  }
}

String WiFiManagerHandlers::getScanItemOut(){
  String page;
  std::vector<const WiFiManager::WiFiScanNetwork*> networks;
  collectVisibleScanResults(networks);
  const int n = static_cast<int>(networks.size());
  reservePage(page, 256 + (n > 0 ? static_cast<size_t>(n) * 96 : 96));

  if (_wm->_scan.state == WiFiManager::WM_SCAN_RUNNING || _wm->_scan.state == WiFiManager::WM_SCAN_QUEUED) {
    page += renderScanMessage(F("Scanning for networks..."));
    return page;
  }

  if (!_wm->_scan.resultsValid || n == 0) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(F("No networks found"));
    #endif
    if (_wm->_scan.state == WiFiManager::WM_SCAN_TIMEOUT) {
      page += renderScanMessage(F("Scan timed out. Refresh to try again."));
    } else if (_wm->_scan.state == WiFiManager::WM_SCAN_FAILED) {
      page += renderScanMessage(F("Scan failed. Refresh to try again."));
    } else {
      page += renderScanMessage(F("No networks found. Refresh to scan again."));
    }
    return page;
  }

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(n,F("networks found"));
  #endif

  String hiddenClass = _wm->_scanDispOptions ? "" : "h";
  String visibleClass = _wm->_scanDispOptions ? "h" : "";

  for (int i = 0; i < n; i++) {
    const auto* network = networks[static_cast<size_t>(i)];

    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("AP: "),(String)network->rssi + " " + network->ssid);
    #endif

    int rssiperc = _wm->getRSSIasQuality(network->rssi);
    String ssid_encoded = _wm->htmlEntities(network->ssid);
    String ssid_display = _wm->htmlEntities(network->ssid, true);
    int quality = int(round(map(rssiperc,0,100,1,4)));
    String lockIcon = (network->encType != WM_WIFIOPEN) ? "l" : "";
    String qualityPercent = String(rssiperc) + "%";

    page += renderScanRow(
      ssid_encoded,
      ssid_display,
      qualityPercent,
      String(quality),
      lockIcon,
      hiddenClass,
      visibleClass,
      qualityPercent
    );

    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("Added network item"));
    #endif
    delay(0);
  }

  page += F("<br/>");
  return page;
}

String WiFiManagerHandlers::getIpForm(String id, String title, String value){
    return renderFieldTemplate(WM_FIELD_LABEL_BEFORE_TEMPLATE, id, title, F("15"), value);
}

String WiFiManagerHandlers::getStaticOut(){
  String page;
  reservePage(page, 384);
  String fields;
  reservePage(fields, 384);
  if ((_wm->_staShowStaticFields || _wm->_sta_static_ip) && _wm->_staShowStaticFields>=0) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV,F("_staShowStaticFields"));
    #endif
    fields += getIpForm(FPSTR(S_ip),F("Static IP"),(_wm->_sta_static_ip ? _wm->_sta_static_ip.toString() : ""));
    fields += getIpForm(FPSTR(S_gw),F("Static gateway"),(_wm->_sta_static_gw ? _wm->_sta_static_gw.toString() : ""));
    fields += getIpForm(FPSTR(S_sn),F("Subnet"),(_wm->_sta_static_sn ? _wm->_sta_static_sn.toString() : ""));
  }

  if((_wm->_staShowDns || _wm->_sta_static_dns) && _wm->_staShowDns>=0){
    fields += getIpForm(FPSTR(S_dns),F("Static DNS"),(_wm->_sta_static_dns ? _wm->_sta_static_dns.toString() : ""));
  }

  if(fields != ""){
    page += renderSectionBreak(fields);
    page += F("<br/>");
  }

  return page;
}

String WiFiManagerHandlers::getParamOut(){
  String page;
  reservePage(page, 256 + (_wm->_paramsCount > 0 ? static_cast<size_t>(_wm->_paramsCount) * 128 : 64));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("getParamOut"),_wm->_paramsCount);
  #endif

  if(_wm->_paramsCount > 0){

    char valLength[5];

    for (int i = 0; i < _wm->_paramsCount; i++) {
      if (_wm->_params[i] == NULL || _wm->_params[i]->getValueLength() > 99999) {
        #ifdef WM_DEBUG_LEVEL
        _wm->DEBUG_WM(WM_DEBUG_ERROR,F("[ERROR] WiFiManagerParameter is out of scope"));
        #endif
        return "";
      }
    }

    // add the extra parameters to the form
    for (int i = 0; i < _wm->_paramsCount; i++) {
      String pitem;
      
      if (_wm->_params[i]->getID() != NULL) {
        String paramId = _wm->_params[i]->getID();
        String paramLabel = _wm->htmlEntities(_wm->_params[i]->getLabel(), true);
        String paramValue = _wm->htmlEntities(_wm->_params[i]->getValue(), true);
        String customHTML = _wm->_params[i]->getCustomHTML();
        snprintf(valLength, 5, "%d", _wm->_params[i]->getValueLength());
        String extraAttrs = customHTML.length() > 0 ? String(F(" ")) + customHTML : String();
        
        switch (_wm->_params[i]->getLabelPlacement()) {
          case WFM_LABEL_BEFORE:
            pitem = renderFieldTemplate(WM_FIELD_LABEL_BEFORE_TEMPLATE, paramId, paramLabel, String(valLength), paramValue, extraAttrs);
            break;
          case WFM_LABEL_AFTER:
            pitem = renderFieldTemplate(WM_FIELD_LABEL_AFTER_TEMPLATE, paramId, paramLabel, String(valLength), paramValue, extraAttrs);
            break;
          default:
            pitem = renderFieldTemplate(WM_FIELD_INPUT_ONLY_TEMPLATE, paramId, paramLabel, String(valLength), paramValue, extraAttrs);
            break;
        }
      } else {
        // Custom HTML only (no ID)
        pitem = _wm->_params[i]->getCustomHTML();
      }

      page += pitem;
    }
  }

  return page;
}

String WiFiManagerHandlers::getInfoData(String id){
  String p;
  if(id==F("uptime")){
    p = renderInfoRow(F("Uptime"), String(millis() / 1000 / 60) + F(" mins ") + String((millis() / 1000) % 60) + F(" secs"));
  }
  else if(id==F("chipid")){
    #ifdef ESP8266
      p = renderInfoRow(F("Chip ID"), String(ESP.getChipId(),HEX));
    #elif defined(ESP32)
      p = renderInfoRow(F("Chip ID"), String((uint32_t)ESP.getEfuseMac(),HEX));
    #endif
  }
  #ifdef ESP32
  else if(id==F("chiprev")){
      String rev = (String)ESP.getChipRevision();
      #ifdef _SOC_EFUSE_REG_H_
        String revb = (String)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S)&&EFUSE_RD_CHIP_VER_RESERVE_V);
        p = renderInfoRow(F("Chip rev"), rev + F("<br/>") + revb);
      #else
        p = renderInfoRow(F("Chip rev"), rev);
      #endif
  }
  #endif
  #ifdef ESP8266
  else if(id==F("fchipid")){
      p = renderInfoRow(F("Flash chip ID"), String(ESP.getFlashChipId()));
  }
  #endif
  else if(id==F("idesize")){
    p = renderInfoRow(F("Flash size"), String(ESP.getFlashChipSize()) + F(" bytes"));
  }
  else if(id==F("flashsize")){
    #ifdef ESP8266
      p = renderInfoRow(F("Real flash size"), String(ESP.getFlashChipRealSize()) + F(" bytes"));
    #elif defined ESP32
      p = renderInfoRow(F("PSRAM Size"), String(ESP.getPsramSize()) + F(" bytes"));
    #endif
  }
  else if(id==F("corever")){
    #ifdef ESP8266
      p = renderInfoRow(F("Core version"), String(ESP.getCoreVersion()));
    #endif      
  }
  #ifdef ESP8266
  else if(id==F("bootver")){
      p = renderInfoRow(F("Boot version"), String(system_get_boot_version()));
  }
  #endif
  else if(id==F("cpufreq")){
    p = renderInfoRow(F("CPU frequency"), String(ESP.getCpuFreqMHz()) + F("MHz"));
  }
  else if(id==F("freeheap")){
    p = renderInfoRow(F("Memory - Free heap"), String(ESP.getFreeHeap()) + F(" bytes available"));
  }
  else if(id==F("memsketch")){
    p = renderInfoRow(F("Memory - Sketch size"), String(F("Used / Total bytes<br/>")) + String(ESP.getSketchSize()) + F(" / ") + String(ESP.getSketchSize()+ESP.getFreeSketchSpace()));
  }
  else if(id==F("memsmeter")){
    p = F("<br/><progress value='");
    p += String(ESP.getSketchSize());
    p += F("' max='");
    p += String(ESP.getSketchSize()+ESP.getFreeSketchSpace());
    p += F("'></progress></dd>");
  }
  else if(id==F("lastreset")){
    #ifdef ESP8266
      p = renderInfoRow(F("Last reset reason"), String(ESP.getResetReason()));
    #elif defined(ESP32) && defined(_ROM_RTC_H_)
      String reasons[2];
      for(int i=0;i<2;i++){
        int reason = rtc_get_reset_reason(i);
        switch (reason)
        {
          case 1  : reasons[i] = F("Vbat power on reset");break;
          case 3  : reasons[i] = F("Software reset digital core");break;
          case 4  : reasons[i] = F("Legacy watch dog reset digital core");break;
          case 5  : reasons[i] = F("Deep Sleep reset digital core");break;
          case 6  : reasons[i] = F("Reset by SLC module, reset digital core");break;
          case 7  : reasons[i] = F("Timer Group0 Watch dog reset digital core");break;
          case 8  : reasons[i] = F("Timer Group1 Watch dog reset digital core");break;
          case 9  : reasons[i] = F("RTC Watch dog Reset digital core");break;
          case 10 : reasons[i] = F("Instrusion tested to reset CPU");break;
          case 11 : reasons[i] = F("Time Group reset CPU");break;
          case 12 : reasons[i] = F("Software reset CPU");break;
          case 13 : reasons[i] = F("RTC Watch dog Reset CPU");break;
          case 14 : reasons[i] = F("for APP CPU, reseted by PRO CPU");break;
          case 15 : reasons[i] = F("Reset when the vdd voltage is not stable");break;
          case 16 : reasons[i] = F("RTC Watch dog reset digital core and rtc module");break;
          default : reasons[i] = F("NO_MEAN");
        }
      }
      p = renderInfoRow(F("Last reset reason"), String(F("CPU0: ")) + reasons[0] + F("<br/>CPU1: ") + reasons[1]);
    #endif
  }
  else if(id==F("apip")){
    p = renderInfoRow(F("Access point IP"), WiFi.softAPIP().toString());
  }
  else if(id==F("apmac")){
    p = renderInfoRow(F("Access point MAC"), WiFi.softAPmacAddress());
  }
  #ifdef ESP32
  else if(id==F("aphost")){
      p = renderInfoRow(F("Access point hostname"), String(WiFi.softAPgetHostname()));
  }
  #endif
  #ifndef WM_NOSOFTAPSSID
  #ifdef ESP8266
  else if(id==F("apssid")){
    p = renderInfoRow(F("Access point SSID"), _wm->htmlEntities(WiFi.softAPSSID(), true));
  }
  #endif
  #endif
  else if(id==F("apbssid")){
    p = renderInfoRow(F("BSSID"), WiFi.BSSIDstr());
  }
  else if(id==F("stassid")){
    p = renderInfoRow(F("Station SSID"), _wm->htmlEntities((String)_wm->WiFi_SSID(), true));
  }
  else if(id==F("staip")){
    p = renderInfoRow(F("Station IP"), WiFi.localIP().toString());
  }
  else if(id==F("stagw")){
    p = renderInfoRow(F("Station gateway"), WiFi.gatewayIP().toString());
  }
  else if(id==F("stasub")){
    p = renderInfoRow(F("Station subnet"), WiFi.subnetMask().toString());
  }
  else if(id==F("dnss")){
    p = renderInfoRow(F("DNS Server"), WiFi.dnsIP().toString());
  }
  else if(id==F("host")){
    #ifdef ESP32
      p = renderInfoRow(F("Hostname"), String(WiFi.getHostname()));
    #else
      p = renderInfoRow(F("Hostname"), String(WiFi.hostname()));
    #endif
  }
  else if(id==F("stamac")){
    p = renderInfoRow(F("Station MAC"), WiFi.macAddress());
  }
  else if(id==F("conx")){
    p = renderInfoRow(F("Connected"), WiFi.isConnected() ? F("Yes") : F("No"));
  }
  #ifdef ESP8266
  else if(id==F("autoconx")){
    p = renderInfoRow(F("Autoconnect"), WiFi.getAutoConnect() ? F("Enabled") : F("Disabled"));
  }
  #endif
  #if defined(ESP32) && !defined(WM_NOTEMP)
  else if(id==F("temp")){
    p = renderInfoRow(F("Temperature"), String(temperatureRead()) + F(" C&deg; / ") + String((temperatureRead()+32)*1.8f) + F(" F&deg;"));
  }
  #endif
  else if(id==F("aboutver")){
    p = renderInfoRow(F("WiFiManager"), FPSTR(WM_VERSION_STR));
  }
  else if(id==F("aboutarduinover")){
    #ifdef VER_ARDUINO_STR
    p = renderInfoRow(F("Arduino"), String(VER_ARDUINO_STR));
    #endif
  }
  else if(id==F("aboutsdkver")){
    #ifdef ESP32
      p = renderInfoRow(F("SDK version"), String(esp_get_idf_version()));
    #else
      p = renderInfoRow(F("SDK version"), String(system_get_sdk_version()));
    #endif
  }
  else if(id==F("aboutdate")){
    p = renderInfoRow(F("Build date"), String(__DATE__ " " __TIME__));
  }
  return p;
}

void WiFiManagerHandlers::reportStatus(String &page){
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("[WIFI] reportStatus prev:"),_wm->getWLStatusString(_wm->_lastconxresult));
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("[WIFI] reportStatus current:"),_wm->getWLStatusString(WiFi.status()));
  String str;
  if (_wm->WiFi_SSID() != ""){
    if (WiFi.status()==WL_CONNECTED){
      str = renderStatusMessage(
        F("Connected"),
        String(F(" to ")) + _wm->htmlEntities(_wm->WiFi_SSID(), true) + F("<br/><em><small>with IP ") + WiFi.localIP().toString() + F("</small></em>"),
        F(" S")
      );
    }
    else {
      String ssidEncoded = _wm->htmlEntities(_wm->WiFi_SSID(), true);
      String statusClass = " D";
      String statusMsg = "";
      
      if(_wm->_lastconxresult == _wm->WL_STATION_WRONG_PASSWORD){
        statusMsg = FPSTR(HTML_STATUS_OFFPW);
      }
      else if(_wm->_lastconxresult == WL_NO_SSID_AVAIL){
        statusMsg = FPSTR(HTML_STATUS_OFFNOAP);
      }
      else if(_wm->_lastconxresult == WL_CONNECT_FAILED || _wm->_lastconxresult == WL_CONNECTION_LOST){
        statusMsg = FPSTR(HTML_STATUS_OFFFAIL);
      }
      else{
        statusClass = "";
      }

      str = renderStatusMessage(F("Not connected"), String(F(" to ")) + ssidEncoded + statusMsg, statusClass);
    }
  }
  else {
    str = FPSTR(HTML_STATUS_NONE);
  }
  page += str;
}

// Captive Portal

boolean WiFiManagerHandlers::captivePortal(AsyncWebServerRequest *request) {
  
  if(!_wm->_enableCaptivePortal || !_wm->configPortalActive) return false;
  
  String serverLoc = _wm->toStringIp(request->client()->localIP());

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, "-> " + request->host());
  _wm->DEBUG_WM(WM_DEBUG_DEV, "serverLoc " + serverLoc);
  #endif

  // fallback for ipv6 bug
  if(serverLoc == "0.0.0.0"){
    if ((WiFi.status()) != WL_CONNECTED)
      serverLoc = _wm->toStringIp(WiFi.softAPIP());
    else
      serverLoc = _wm->toStringIp(WiFi.localIP());
  }
  
  if(_wm->_httpPort != 80) serverLoc += ":" + (String)_wm->_httpPort;
  bool doredirect = serverLoc != request->host();
  
  if (doredirect) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Request redirected to captive portal"));
    _wm->DEBUG_WM(WM_DEBUG_DEV, "serverLoc " + serverLoc);
    _wm->DEBUG_WM(WM_DEBUG_DEV, "Original URL " + request->url());
    #endif
    String redirectUrl = (String)F("http://") + serverLoc + request->url();
    if (request->params() > 0) {
      redirectUrl += F("?");
      for (size_t i = 0; i < request->params(); i++) {
        if (i > 0) redirectUrl += F("&");
        redirectUrl += request->getParam(i)->name() + F("=") + request->getParam(i)->value();
      }
    }
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, "Redirect URL " + redirectUrl);
    #endif
    request->redirect(redirectUrl);
    return true;
  }
  return false;
}

void WiFiManagerHandlers::stopCaptivePortal(){
  _wm->_enableCaptivePortal = false;
}

// HTTP Handlers

void WiFiManagerHandlers::handleRequest(AsyncWebServerRequest *request) {
  _wm->_webPortalAccessed = millis();
}

void WiFiManagerHandlers::handleRoot(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Root"));
  #endif
  if (captivePortal(request)) return;
  handleRequest(request);

  AsyncWebServerResponse *response = nullptr;
  auto bundle = std::make_shared<RootRenderBundle>();
  buildRootState(this, bundle->state);

  if (_wm->_serverManager) {
    _wm->_serverManager->registerDefaultStyles(bundle->registry);
    _wm->_serverManager->registerDefaultScripts(bundle->registry);
    _wm->_serverManager->registerDefaultPageTitle(bundle->registry);
    _wm->_serverManager->registerDefaultSubtitle(bundle->registry);
    _wm->_serverManager->applyTemplateSetupCallback(bundle->registry);
  }

  if (bundle->registry.getPlaceholder("%MENU%") == nullptr) {
    configureDynamicStringDescriptor(bundle->menuDescriptor, bundle->state.menu);
    bundle->registry.registerDynamicTemplate("%MENU%", &bundle->menuDescriptor);
  }

  if (bundle->registry.getPlaceholder("%STATUS%") == nullptr) {
    configureDynamicStringDescriptor(bundle->statusDescriptor, bundle->state.status);
    bundle->registry.registerDynamicTemplate("%STATUS%", &bundle->statusDescriptor);
  }

  response = beginTemplateResponse(request, bundle, WM_ROOT_TEMPLATE);

  request->send(response);
}

void WiFiManagerHandlers::handleWifi(AsyncWebServerRequest *request, boolean scan) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Wifi"));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("handleWifi called, scan="), scan ? "true" : "false");
  #endif
  if (captivePortal(request)) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("Captive portal redirect"));
    #endif
    return;
  }
  handleRequest(request);
  auto bundle = std::make_shared<WiFiRenderBundle>();

  String ssidPlaceholder = _wm->WiFi_SSID();
  String passwordPlaceholder = "";
  if(_wm->_showPassword){
    passwordPlaceholder = _wm->WiFi_psk();
  }
  else if(_wm->WiFi_psk() != ""){
    passwordPlaceholder = F("********");
  }

  buildWiFiPageState(this,
                     bundle->state,
                     scan,
                     _wm->_showBack,
                     ssidPlaceholder,
                     passwordPlaceholder,
                     _wm->_paramsInWifi && _wm->_paramsCount > 0);

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  registerShellTemplate(bundle->registry,
                        bundle->docTitleDescriptor,
                        bundle->state.docTitle,
                        C_wifi,
                        WM_WIFI_CONTENT_TEMPLATE,
                        WIFI_POLLING_JS);
  configureDynamicStringDescriptor(bundle->scanContentDescriptor, bundle->state.scanContent);
  configureDynamicStringDescriptor(bundle->ssidPlaceholderDescriptor, bundle->state.ssidPlaceholder);
  configureDynamicStringDescriptor(bundle->passwordPlaceholderDescriptor, bundle->state.passwordPlaceholder);
  configureDynamicStringDescriptor(bundle->staticFieldsDescriptor, bundle->state.staticFields);
  configureDynamicStringDescriptor(bundle->paramSectionDescriptor, bundle->state.paramSection);
  configureDynamicStringDescriptor(bundle->formActionsDescriptor, bundle->state.formActions);
  configureDynamicStringDescriptor(bundle->pageActionsDescriptor, bundle->state.pageActions);
  configureDynamicStringDescriptor(bundle->statusDescriptor, bundle->state.status);

  bundle->registry.registerDynamicTemplate("%WIFI_SCAN_CONTENT%", &bundle->scanContentDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_SSID_PLACEHOLDER%", &bundle->ssidPlaceholderDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_PASSWORD_PLACEHOLDER%", &bundle->passwordPlaceholderDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_STATIC_FIELDS%", &bundle->staticFieldsDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_PARAM_SECTION%", &bundle->paramSectionDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_FORM_ACTIONS%", &bundle->formActionsDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_PAGE_ACTIONS%", &bundle->pageActionsDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_STATUS%", &bundle->statusDescriptor);

  #ifdef WM_DEBUG_LEVEL
  size_t debugPageLength = bundle->state.scanContent.length() + bundle->state.staticFields.length()
                         + bundle->state.paramSection.length()
                         + bundle->state.formActions.length() + bundle->state.pageActions.length()
                         + bundle->state.status.length();
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Page length: "), String(debugPageLength));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_numNetworks: "), String(_wm->_numNetworks));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_scanState: "), String(_wm->_scan.state));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_lastscan: "), String(_wm->_lastscan));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("About to send response"));
  #endif

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Response sent"));
  #endif
}

void WiFiManagerHandlers::handleParam(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Param"));
  #endif
  handleRequest(request);
  auto bundle = std::make_shared<ParamRenderBundle>();
  buildParamPageState(this, bundle->state, _wm->_showBack);

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  registerShellTemplate(bundle->registry,
                        bundle->docTitleDescriptor,
                        bundle->state.docTitle,
                        C_param,
                        WM_PARAM_CONTENT_TEMPLATE);
  configureDynamicStringDescriptor(bundle->fieldsDescriptor, bundle->state.fields);
  configureDynamicStringDescriptor(bundle->formActionsDescriptor, bundle->state.formActions);
  configureDynamicStringDescriptor(bundle->pageActionsDescriptor, bundle->state.pageActions);
  configureDynamicStringDescriptor(bundle->statusDescriptor, bundle->state.status);

  bundle->registry.registerDynamicTemplate("%PARAM_FIELDS%", &bundle->fieldsDescriptor);
  bundle->registry.registerDynamicTemplate("%PARAM_FORM_ACTIONS%", &bundle->formActionsDescriptor);
  bundle->registry.registerDynamicTemplate("%PARAM_PAGE_ACTIONS%", &bundle->pageActionsDescriptor);
  bundle->registry.registerDynamicTemplate("%PARAM_STATUS%", &bundle->statusDescriptor);

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent param page"));
  #endif
}

void WiFiManagerHandlers::handleWifiSave(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi save "));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Method:"), request->method() == HTTP_GET ? F("GET") : F("POST"));
  #endif
  handleRequest(request);

  WiFiManager::WiFiManagerRequestArgs requestArgs(request);

  if (request->hasParam("s", true)) {
    _wm->_ssid = request->getParam("s", true)->value().c_str();
  }
  if (request->hasParam("p", true)) {
    _wm->_pass = request->getParam("p", true)->value().c_str();
  }

  if(_wm->_ssid == "" && _wm->_pass != ""){
    _wm->_ssid = _wm->WiFi_SSID(true);
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("Detected WiFi password change"));
    #endif    
  }

  #ifdef WM_DEBUG_LEVEL
  String requestinfo = "SERVER_REQUEST\n----------------\n";
  requestinfo += "URI: ";
  requestinfo += request->url();
  requestinfo += "\nMethod: ";
  requestinfo += (request->method() == HTTP_GET) ? "GET" : "POST";
  requestinfo += "\nArguments: ";
  requestinfo += request->params();
  requestinfo += "\n";
    for (size_t i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      requestinfo += " " + p->name() + ": " + p->value() + "\n";
    }

  _wm->DEBUG_WM(WM_DEBUG_MAX, requestinfo);
  #endif

  if (request->hasParam(FPSTR(S_ip), true)) {
    String ip = request->getParam(FPSTR(S_ip), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_ip, ip.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static ip:"), ip);
    #endif
  }
  if (request->hasParam(FPSTR(S_gw), true)) {
    String gw = request->getParam(FPSTR(S_gw), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_gw, gw.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static gateway:"), gw);
    #endif
  }
  if (request->hasParam(FPSTR(S_sn), true)) {
    String sn = request->getParam(FPSTR(S_sn), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_sn, sn.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static netmask:"), sn);
    #endif
  }
  if (request->hasParam(FPSTR(S_dns), true)) {
    String dns = request->getParam(FPSTR(S_dns), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_dns, dns.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static DNS:"), dns);
    #endif
  }

  if (_wm->_presavewificallback != NULL) {
    _wm->_presavewificallback();
  }

  if(_wm->_paramsInWifi) doParamSave(requestArgs);

  auto bundle = std::make_shared<MessageRenderBundle>();
  String messageBody;
  String actions;
  reservePage(messageBody, 1024);

  if(_wm->_ssid == ""){
    messageBody += FPSTR(HTML_PARAMSAVED);
  }
  else {
    messageBody += FPSTR(HTML_SAVED);
  }

  if(_wm->_showBack) actions += renderActionForm(F("/"), F("get"), F("Back"), String(), F("<hr><br/>"));

  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              _wm->_ssid == "" ? F("Settings saved") : F("Credentials saved"),
                              C_wifi,
                              messageBody,
                              actions);

  AsyncWebServerResponse *response = beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE);
  response->addHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
  request->send(response);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent wifi save page"));
  #endif

  _wm->connect = true;
}

void WiFiManagerHandlers::handleParamSave(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Param save "));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Method:"), request->method() == HTTP_GET ? F("GET") : F("POST"));
  #endif
  handleRequest(request);

  WiFiManager::WiFiManagerRequestArgs requestArgs(request);

  doParamSave(requestArgs);

  auto bundle = std::make_shared<MessageRenderBundle>();
  String messageBody;
  String actions;
  reservePage(messageBody, 1024);
  messageBody += FPSTR(HTML_PARAMSAVED);
  if(_wm->_showBack) actions += renderActionForm(F("/"), F("get"), F("Back"), String(), F("<hr><br/>"));

  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Setup saved"),
                              C_param,
                              messageBody,
                              actions);

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent param save page"));
  #endif
}

void WiFiManagerHandlers::doParamSave(WiFiManager::WiFiManagerRequestArgs requestArgs){
  if ( _wm->_presaveparamscallback != NULL) {
    _wm->_presaveparamscallback();
  }

  if(_wm->_paramsCount > 0){
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("Parameters"));
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("--------------------"));
    #endif

    for (int i = 0; i < _wm->_paramsCount; i++) {
      if (_wm->_params[i] == NULL || _wm->_params[i]->getValueLength() > 99999) {
        #ifdef WM_DEBUG_LEVEL
        _wm->DEBUG_WM(WM_DEBUG_ERROR,F("[ERROR] WiFiManagerParameter is out of scope"));
        #endif
        break;
      }
      
      String value;
      
      if (_wm->_params[i]->getID() == nullptr) {
        String name = "param_" + String(i);
        
        if (requestArgs.hasArg(name)) {
          value = requestArgs.getArg(name);
        } else {
          continue;
        }
      } else {
        String name = "param_" + String(i);
        if(requestArgs.hasArg(name)) {
          value = requestArgs.getArg(name);
        } else {
          value = requestArgs.getArg(_wm->_params[i]->getID());
        }
      }

      _wm->_params[i]->setValue(value.c_str(), value.length());
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_VERBOSE,(String)_wm->_params[i]->getID() + ":",value);
      #endif
    }
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("--------------------"));
    #endif
  }

   if ( _wm->_saveparamscallback != NULL) {
    _wm->_saveparamscallback(requestArgs);
  }
   
}

void WiFiManagerHandlers::handleInfo(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Info"));
  #endif
  handleRequest(request);
  auto bundle = std::make_shared<InfoRenderBundle>();
  buildInfoPageState(this, bundle->state, _wm->_showInfoUpdate, _wm->_showInfoErase, _wm->_showBack);

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  registerShellTemplate(bundle->registry,
                        bundle->docTitleDescriptor,
                        bundle->state.docTitle,
                        C_info,
                        WM_INFO_CONTENT_TEMPLATE);
  configureDynamicStringDescriptor(bundle->statusDescriptor, bundle->state.status);
  configureDynamicStringDescriptor(bundle->deviceDescriptor, bundle->state.deviceSection);
  configureDynamicStringDescriptor(bundle->wifiDescriptor, bundle->state.wifiSection);
  configureDynamicStringDescriptor(bundle->aboutDescriptor, bundle->state.aboutSection);
  configureDynamicStringDescriptor(bundle->footerDescriptor, bundle->state.footer);

  bundle->registry.registerDynamicTemplate("%INFO_STATUS%", &bundle->statusDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_DEVICE_SECTION%", &bundle->deviceDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_WIFI_SECTION%", &bundle->wifiDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_ABOUT_SECTION%", &bundle->aboutDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_FOOTER%", &bundle->footerDescriptor);

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent info page"));
  #endif
}

void WiFiManagerHandlers::handleExit(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Exit"));
  #endif
  handleRequest(request);
  auto bundle = std::make_shared<MessageRenderBundle>();
  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Exit"),
                              C_exit,
                              String(F("Exiting")));

  AsyncWebServerResponse *response = beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE);
  response->addHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  request->send(response);
  
  _wm->_abortScheduled = true;
  _wm->_abortTime = millis() + _wm->EXIT_DELAY_MS;
}

void WiFiManagerHandlers::handleReset(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Reset"));
  #endif
  handleRequest(request);
  auto bundle = std::make_shared<MessageRenderBundle>();
  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Reset"),
                              C_restart,
                              String(F("Module will reset in a few seconds.")));

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(F("RESETTING ESP"));
  #endif
  _wm->_rebootScheduled = true;
  _wm->_rebootTime = millis() + _wm->REBOOT_DELAY_MS;
}

void WiFiManagerHandlers::handleErase(AsyncWebServerRequest *request, boolean opt) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_NOTIFY, F("<- HTTP Erase"));
  #endif
  handleRequest(request);
  bool ret = _wm->erase(opt);
  auto bundle = std::make_shared<MessageRenderBundle>();
  String messageBody;
  reservePage(messageBody, 256);

  if(ret) messageBody += F("Module will reset in a few seconds.");
  else {
    messageBody += F("An error occured");
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] WiFi EraseConfig failed"));
    #endif
  }

  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Erase"),
                              C_erase,
                              messageBody);

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  if(ret){
    _wm->_rebootScheduled = true;
    _wm->_rebootTime = millis() + _wm->ERASE_REBOOT_DELAY_MS;
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(F("RESETTING ESP"));
    #endif
  }	
}

void WiFiManagerHandlers::handleClose(AsyncWebServerRequest *request){
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("Disabling Captive Portal"));
  stopCaptivePortal();
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP close"));
  #endif
  handleRequest(request);
  auto bundle = std::make_shared<MessageRenderBundle>();
  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Close"),
                              C_close,
                              String(F("You can close the page, portal will continue to run")));

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));
}

void WiFiManagerHandlers::handleNotFound(AsyncWebServerRequest *request) {
  if (captivePortal(request)) return;
  handleRequest(request);
  String message = F("File not found\n\n");
  AsyncWebServerResponse *response = request->beginResponse(404, FPSTR(HTTP_HEAD_CT2), message);
  response->addHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  response->addHeader(F("Pragma"), F("no-cache"));
  response->addHeader(F("Expires"), F("-1"));
  request->send(response);
}

void WiFiManagerHandlers::handleWiFiScanStatus(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi scan status"));
  #endif
  handleRequest(request);

  std::vector<const WiFiManager::WiFiScanNetwork*> networks;
  collectVisibleScanResults(networks);

  String json = "{";
  reservePage(json, 160 + (networks.size() > 0 ? networks.size() * 96 : 64));
  json += "\"state\":\"";
  switch (_wm->_scan.state) {
    case WiFiManager::WM_SCAN_IDLE: json += "idle"; break;
    case WiFiManager::WM_SCAN_QUEUED: json += "queued"; break;
    case WiFiManager::WM_SCAN_RUNNING: json += "running"; break;
    case WiFiManager::WM_SCAN_COMPLETE: json += "complete"; break;
    case WiFiManager::WM_SCAN_FAILED: json += "failed"; break;
    case WiFiManager::WM_SCAN_TIMEOUT: json += "timeout"; break;
  }
  json += "\",\"scanning\":";
  json += (_wm->_scan.state == WiFiManager::WM_SCAN_RUNNING || _wm->_scan.state == WiFiManager::WM_SCAN_QUEUED) ? "true" : "false";
  json += ",\"results_valid\":";
  json += _wm->_scan.resultsValid ? "true" : "false";
  json += ",\"count\":";
  json += String(networks.size());
  json += ",\"lastscan\":";
  json += String(_wm->_lastscan);
  json += ",\"error\":\"";
  if (_wm->_scan.state == WiFiManager::WM_SCAN_TIMEOUT) {
    json += "timeout";
  } else if (_wm->_scan.state == WiFiManager::WM_SCAN_FAILED) {
    json += "failed";
  }
  json += "\",\"networks\":[";
  appendVisibleScanResultsJson(json, networks);
  json += "]}";

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

void WiFiManagerHandlers::appendVisibleScanResultsJson(String& json, const std::vector<const WiFiManager::WiFiScanNetwork*>& networks) {
  for (size_t i = 0; i < networks.size(); i++) {
    const auto* network = networks[i];
    if (i > 0) {
      json += ",";
    }
    json += "{";
    json += "\"ssid\":\"";
    json += _wm->htmlEntities(network->ssid, true);
    json += "\",\"rssi\":";
    json += String(network->rssi);
    json += ",\"quality\":";
    json += String(_wm->getRSSIasQuality(network->rssi));
    json += ",\"enc_type\":";
    json += String(network->encType);
    json += ",\"encrypted\":";
    json += (network->encType != WM_WIFIOPEN) ? "true" : "false";
    json += "}";
  }
}

void WiFiManagerHandlers::handleWiFiScanRequest(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi scan request"));
  #endif
  handleRequest(request);
  _wm->requestAsyncScan(true);

  AsyncWebServerResponse *response = request->beginResponse(202, "application/json",
    "{\"accepted\":true,\"state\":\"queued\"}");
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

void WiFiManagerHandlers::handleUpdate(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Handle update"));
  #endif
  if (captivePortal(request)) return;
  handleRequest(request);
  auto bundle = std::make_shared<MessageRenderBundle>();
  const String subtitle = _wm->configPortalActive
    ? _wm->_apName
    : (_wm->getWiFiHostname() + " - " + WiFi.localIP().toString());
  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Update"),
                              C_update,
                              buildUpdatePanelContent(_wm->_title, subtitle));

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));
}

void WiFiManagerHandlers::handleUpdating(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static unsigned long _configPortalTimeoutSAV = 0;
  static bool timeoutSaved = false;
  
  if (!index) {
    if (!timeoutSaved) {
      _configPortalTimeoutSAV = _wm->_configPortalTimeout;
      timeoutSaved = true;
    }
    _wm->_configPortalTimeout = 0;
    
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("[OTA] Update file: "), filename.c_str());
    #endif
    
    if (_wm->_preotaupdatecallback != NULL) {
      _wm->_preotaupdatecallback();
    }
    
    #ifdef ESP8266
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    #elif defined(ESP32)
      uint32_t maxSketchSpace = UPDATE_SIZE_UNKNOWN;
    #endif
    
    if (!Update.begin(maxSketchSpace)) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] OTA Update ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.begin failed");
      _wm->_configPortalTimeout = _configPortalTimeoutSAV;
      timeoutSaved = false;
      return;
    }
  }
  
  if (len) {
    if (Update.write(data, len) != len) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] OTA Update WRITE ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.write failed");
      _wm->_configPortalTimeout = _configPortalTimeoutSAV;
      timeoutSaved = false;
      return;
    }
  }
  
  if (final) {
    if (Update.end(true)) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("\n\n[OTA] OTA FILE END bytes: "), (String)index);
      #endif
    } else {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] OTA Update END ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.end failed");
    }
    
    _wm->_configPortalTimeout = _configPortalTimeoutSAV;
    timeoutSaved = false;
  }
}

void WiFiManagerHandlers::handleUpdateDone(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Handle update done"));
  #endif
  handleRequest(request);

  auto bundle = std::make_shared<MessageRenderBundle>();
  const String subtitle = _wm->configPortalActive ? _wm->_apName : WiFi.localIP().toString();
  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Update"),
                              C_update,
                              buildUpdateResultContent(_wm->_title, subtitle));

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  if (!Update.hasError()) {
    delay(1000);
    ESP.restart();
  }
}

#endif // defined(ESP8266) || defined(ESP32)

