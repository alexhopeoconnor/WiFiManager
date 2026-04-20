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
#include "templates/PortalAppJS.h"
#include <TemplateEngine.h>
#include <cstring>

#if defined(ESP8266) || defined(ESP32)

#ifndef WM_PAGE_RESERVE_BYTES
#define WM_PAGE_RESERVE_BYTES 8192
#endif

static void jsonAppendEscaped(String& out, const String& s) {
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '"') {
      out += F("\\\"");
    } else if (c == '\\') {
      out += F("\\\\");
    } else if (c == '\n') {
      out += F("\\n");
    } else if (c == '\r') {
      // skip
    } else {
      out += c;
    }
  }
}

/** Parse a single `<dt>...</dt><dd>...</dd>` row from `getInfoData` HTML. */
static bool parseInfoRowDtDd(const String& html, String& label, String& value) {
  const int dtOpen = html.indexOf(F("<dt>"));
  if (dtOpen < 0) {
    return false;
  }
  const int dtClose = html.indexOf(F("</dt>"), dtOpen);
  if (dtClose < 0) {
    return false;
  }
  label = html.substring(dtOpen + 4, dtClose);
  const int ddOpen = html.indexOf(F("<dd>"), dtClose);
  if (ddOpen < 0) {
    return false;
  }
  const int ddClose = html.indexOf(F("</dd>"), ddOpen);
  if (ddClose < 0) {
    return false;
  }
  value = html.substring(ddOpen + 4, ddClose);
  return true;
}

static void appendJsonInfoArrayFromIds(WiFiManagerHandlers* handlers,
                                       String& json,
                                       const char* const* ids,
                                       size_t count,
                                       bool& first) {
  for (size_t i = 0; i < count; i++) {
    const String row = handlers->getInfoData(ids[i]);
    if (row.length() == 0) {
      continue;
    }
    String label;
    String value;
    if (!parseInfoRowDtDd(row, label, value)) {
      continue;
    }
    if (!first) {
      json += F(",");
    }
    first = false;
    json += F("{\"key\":\"");
    json += ids[i];
    json += F("\",\"label\":\"");
    jsonAppendEscaped(json, label);
    json += F("\",\"value\":\"");
    jsonAppendEscaped(json, value);
    json += F("\"}");
  }
}

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

String renderActionForm(const String& action,
                        const String& method,
                        const String& label,
                        const String& buttonClassAttr = String(),
                        const String& prefix = String(),
                        const String& suffix = String()) {
  String output;
  reservePage(output, prefix.length() + suffix.length() + action.length() + method.length() +
                        label.length() + buttonClassAttr.length() + 48);
  output += prefix;
  output += F("<form action='");
  output += action;
  output += F("' method='");
  output += method;
  output += F("'><button");
  output += buttonClassAttr;
  output += F(">");
  output += label;
  output += F("</button></form>");
  output += suffix;
  return output;
}

String renderSubmitButton(const String& label) {
  String output;
  reservePage(output, label.length() + 40);
  output += F("<br/><br/><button type='submit'>");
  output += label;
  output += F("</button>");
  return output;
}

String renderCenteredButton(const String& id,
                            const String& type,
                            const String& onClick,
                            const String& label,
                            const String& extraAttrs = String()) {
  String output;
  reservePage(output, id.length() + type.length() + onClick.length() + label.length() +
                        extraAttrs.length() + 72);
  output += F("<br/><div class='c'><button id='");
  output += id;
  output += F("' type='");
  output += type;
  output += F("' onclick='");
  output += onClick;
  output += F("'");
  output += extraAttrs;
  output += F(">");
  output += label;
  output += F("</button></div>");
  return output;
}

String renderSectionBreak(const String& content) {
  if (content.length() == 0) {
    return String();
  }

  String output;
  reservePage(output, content.length() + 16);
  output += F("<hr><br/>");
  output += content;
  return output;
}

String renderScanMessage(const String& message) {
  String output;
  reservePage(output, message.length() + 16);
  output += message;
  output += F("<br/><br/>");
  return output;
}

String renderScanRow(const String& ssidAttr,
                     const String& ssidText,
                     const String& qualityLabel,
                     const String& qualityIcon,
                     const String& lockClass,
                     const String& iconVisibilityClass,
                     const String& valueVisibilityClass,
                     const String& qualityValue) {
  String output;
  reservePage(output, ssidAttr.length() + ssidText.length() + qualityLabel.length() +
                        qualityIcon.length() + lockClass.length() +
                        iconVisibilityClass.length() + valueVisibilityClass.length() +
                        qualityValue.length() + 160);
  output += F("<div><a href='#p' onclick='c(this)' data-ssid='");
  output += ssidAttr;
  output += F("'>");
  output += ssidText;
  output += F("</a><div role='img' aria-label='");
  output += qualityLabel;
  output += F("' title='");
  output += qualityLabel;
  output += F("' class='q q-");
  output += qualityIcon;
  output += F(" ");
  output += lockClass;
  output += F(" ");
  output += iconVisibilityClass;
  output += F("'></div><div class='q ");
  output += valueVisibilityClass;
  output += F("'>");
  output += qualityValue;
  output += F("</div></div>");
  return output;
}

String renderFieldTemplate(const char* templateData,
                           const String& id,
                           const String& label,
                           const String& maxLength,
                           const String& value,
                           const String& extraAttrs = String()) {
  String input;
  reservePage(input, id.length() + label.length() + maxLength.length() + value.length() +
                       extraAttrs.length() + 96);
  input += F("<input id='");
  input += id;
  input += F("' name='");
  input += id;
  input += F("' maxlength='");
  input += maxLength;
  input += F("' value='");
  input += value;
  input += F("'");
  input += extraAttrs;
  input += F(">");

  if (templateData == WM_FIELD_LABEL_BEFORE_TEMPLATE) {
    String output;
    reservePage(output, label.length() + input.length() + 32);
    output += F("<label for='");
    output += id;
    output += F("'>");
    output += label;
    output += F("</label><br/>");
    output += input;
    return output;
  }

  if (templateData == WM_FIELD_LABEL_AFTER_TEMPLATE) {
    String output;
    reservePage(output, label.length() + input.length() + 32);
    output += F("<br/>");
    output += input;
    output += F("<label for='");
    output += id;
    output += F("'>");
    output += label;
    output += F("</label>");
    return output;
  }

  String output;
  reservePage(output, input.length() + 8);
  output += F("<br/>");
  output += input;
  return output;
}

String renderInfoRow(const String& label, const String& value) {
  String output;
  reservePage(output, label.length() + value.length() + 24);
  output += F("<dt>");
  output += label;
  output += F("</dt><dd>");
  output += value;
  output += F("</dd>");
  return output;
}

String renderInfoSection(const String& title,
                         const String& rows,
                         const String& prefix = String(),
                         const String& suffix = String()) {
  String output;
  reservePage(output, prefix.length() + suffix.length() + title.length() + rows.length() + 40);
  output += prefix;
  output += F("<h3>");
  output += title;
  output += F("</h3><hr><dl>");
  output += rows;
  output += F("</dl>");
  output += suffix;
  return output;
}

String renderStatusMessage(const String& title,
                           const String& body,
                           const String& statusClassSuffix = String()) {
  String output;
  reservePage(output, title.length() + body.length() + statusClassSuffix.length() + 48);
  output += F("<div class='msg");
  output += statusClassSuffix;
  output += F("'><strong>");
  output += title;
  output += F("</strong>");
  output += body;
  output += F("</div>");
  return output;
}

String renderPageHeading(const String& title, const String& subtitle) {
  String output;
  reservePage(output, title.length() + subtitle.length() + 20);
  output += F("<h1>");
  output += title;
  output += F("</h1><h3>");
  output += subtitle;
  output += F("</h3>");
  return output;
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

struct ShellRenderBundle {
  String bootstrapJson;
  String pageTitleStatic;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor bootstrapDescriptor;
  DynamicTemplateDescriptor pageTitleDescriptor;

  ShellRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY), bootstrapDescriptor{}, pageTitleDescriptor{} {}
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
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("No networks found"));
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

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, n,F("networks found"));
  #endif

  String hiddenClass = _wm->_scanDispOptions ? "" : "h";
  String visibleClass = _wm->_scanDispOptions ? "h" : "";

  for (int i = 0; i < n; i++) {
    const auto* network = networks[static_cast<size_t>(i)];

    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("AP: "),(String)network->rssi + " " + network->ssid);
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

    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("Added network item"));
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
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("_staShowStaticFields"));
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

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("getParamOut"),_wm->_paramsCount);
  #endif

  if(_wm->_paramsCount > 0){

    char valLength[5];

    for (int i = 0; i < _wm->_paramsCount; i++) {
      if (_wm->_params[i] == NULL || _wm->_params[i]->getValueLength() > 99999) {
        #ifndef WM_NO_LOG
        _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] WiFiManagerParameter is out of scope"));
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
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("[WIFI] reportStatus prev:"),_wm->getWLStatusString(_wm->_lastconxresult));
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("[WIFI] reportStatus current:"),_wm->getWLStatusString(WiFi.status()));
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

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, "-> " + request->host());
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, "serverLoc " + serverLoc);
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
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- Request redirected to captive portal"));
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, "serverLoc " + serverLoc);
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, "Original URL " + request->url());
    #endif
    String redirectUrl = (String)F("http://") + serverLoc + request->url();
    if (request->params() > 0) {
      redirectUrl += F("?");
      for (size_t i = 0; i < request->params(); i++) {
        if (i > 0) redirectUrl += F("&");
        redirectUrl += request->getParam(i)->name() + F("=") + request->getParam(i)->value();
      }
    }
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, "Redirect URL " + redirectUrl);
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
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Root"));
  #endif
  if (captivePortal(request)) return;
  handleRequest(request);

  auto bundle = std::make_shared<ShellRenderBundle>();
  bundle->bootstrapJson = buildPortalBootstrapJson();

  if (_wm->_serverManager) {
    _wm->_serverManager->registerDefaultStyles(bundle->registry);
    _wm->_serverManager->registerDefaultScripts(bundle->registry);
    _wm->_serverManager->registerDefaultPageTitle(bundle->registry);
    _wm->_serverManager->applyTemplateSetupCallback(bundle->registry);
  } else {
    registerSharedShellPlaceholders(nullptr, bundle->registry);
    bundle->pageTitleStatic = _wm->_title;
    configureDynamicStringDescriptor(bundle->pageTitleDescriptor, bundle->pageTitleStatic);
    bundle->registry.registerDynamicTemplate("%PAGE_TITLE%", &bundle->pageTitleDescriptor);
  }

  configureDynamicStringDescriptor(bundle->bootstrapDescriptor, bundle->bootstrapJson);
  bundle->registry.registerDynamicTemplate("%BOOTSTRAP_JSON%", &bundle->bootstrapDescriptor);
  bundle->registry.registerProgmemData("%PORTAL_APP_JS%", PORTAL_APP_JS);

  request->send(beginTemplateResponse(request, bundle, WM_ROOT_SHELL_TEMPLATE));
}

void WiFiManagerHandlers::handleWifi(AsyncWebServerRequest *request, boolean scan) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Wifi"));
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("handleWifi called, scan="), scan ? "true" : "false");
  #endif
  (void)scan;
  if (captivePortal(request)) {
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("Captive portal redirect"));
    #endif
    return;
  }
  handleRequest(request);
  request->redirect(String(F("/#/wifi")));
}

void WiFiManagerHandlers::handleParam(AsyncWebServerRequest *request){
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Param"));
  #endif
  handleRequest(request);
  request->redirect(String(F("/#/setup")));
}

void WiFiManagerHandlers::applyWifiAndParamsFromRequest(AsyncWebServerRequest *request) {
  WiFiManager::WiFiManagerRequestArgs requestArgs(request);

  if (request->hasParam("s", true)) {
    _wm->_ssid = request->getParam("s", true)->value().c_str();
  }
  if (request->hasParam("p", true)) {
    _wm->_pass = request->getParam("p", true)->value().c_str();
  }

  if (_wm->_ssid == "" && _wm->_pass != "") {
    _wm->_ssid = _wm->WiFi_SSID(true);
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("Detected WiFi password change"));
#endif
  }

#ifndef WM_NO_LOG
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

  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, requestinfo);
#endif

  if (request->hasParam(FPSTR(S_ip), true)) {
    String ip = request->getParam(FPSTR(S_ip), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_ip, ip.c_str());
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("static ip:"), ip);
#endif
  }
  if (request->hasParam(FPSTR(S_gw), true)) {
    String gw = request->getParam(FPSTR(S_gw), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_gw, gw.c_str());
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("static gateway:"), gw);
#endif
  }
  if (request->hasParam(FPSTR(S_sn), true)) {
    String sn = request->getParam(FPSTR(S_sn), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_sn, sn.c_str());
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("static netmask:"), sn);
#endif
  }
  if (request->hasParam(FPSTR(S_dns), true)) {
    String dns = request->getParam(FPSTR(S_dns), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_dns, dns.c_str());
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("static DNS:"), dns);
#endif
  }

  if (_wm->_presavewificallback != NULL) {
    _wm->_presavewificallback();
  }

  if (_wm->_paramsInWifi) {
    doParamSave(requestArgs);
  }
}

void WiFiManagerHandlers::handleWifiSave(AsyncWebServerRequest *request) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP WiFi save "));
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("Method:"), request->method() == HTTP_GET ? F("GET") : F("POST"));
  #endif
  handleRequest(request);

  applyWifiAndParamsFromRequest(request);

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

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("Sent wifi save page"));
  #endif

  _wm->connect = true;
}

void WiFiManagerHandlers::handleParamSave(AsyncWebServerRequest *request) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Param save "));
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("Method:"), request->method() == HTTP_GET ? F("GET") : F("POST"));
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

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("Sent param save page"));
  #endif
}

void WiFiManagerHandlers::doParamSave(WiFiManager::WiFiManagerRequestArgs requestArgs){
  if ( _wm->_presaveparamscallback != NULL) {
    _wm->_presaveparamscallback();
  }

  if(_wm->_paramsCount > 0){
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Parameters"));
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("--------------------"));
    #endif

    for (int i = 0; i < _wm->_paramsCount; i++) {
      if (_wm->_params[i] == NULL || _wm->_params[i]->getValueLength() > 99999) {
        #ifndef WM_NO_LOG
        _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] WiFiManagerParameter is out of scope"));
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
      #ifndef WM_NO_LOG
      _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,(String)_wm->_params[i]->getID() + ":",value);
      #endif
    }
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("--------------------"));
    #endif
  }

   if ( _wm->_saveparamscallback != NULL) {
    _wm->_saveparamscallback(requestArgs);
  }
   
}

void WiFiManagerHandlers::handleInfo(AsyncWebServerRequest *request) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Info"));
  #endif
  handleRequest(request);
  request->redirect(String(F("/#/info")));
}

void WiFiManagerHandlers::handleExit(AsyncWebServerRequest *request) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Exit"));
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
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP Reset"));
  #endif
  handleRequest(request);
  auto bundle = std::make_shared<MessageRenderBundle>();
  sendMessageTemplateResponse(_wm->_serverManager.get(),
                              bundle,
                              F("Reset"),
                              C_restart,
                              String(F("Module will reset in a few seconds.")));

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("RESETTING ESP"));
  #endif
  _wm->_rebootScheduled = true;
  _wm->_rebootTime = millis() + _wm->REBOOT_DELAY_MS;
}

void WiFiManagerHandlers::handleErase(AsyncWebServerRequest *request, boolean opt) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("<- HTTP Erase"));
  #endif
  handleRequest(request);
  bool ret = _wm->erase(opt);
  auto bundle = std::make_shared<MessageRenderBundle>();
  String messageBody;
  reservePage(messageBody, 256);

  if(ret) messageBody += F("Module will reset in a few seconds.");
  else {
    messageBody += F("An error occured");
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] WiFi EraseConfig failed"));
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
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("RESETTING ESP"));
    #endif
  }	
}

void WiFiManagerHandlers::handleClose(AsyncWebServerRequest *request){
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("Disabling Captive Portal"));
  stopCaptivePortal();
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP close"));
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
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP WiFi scan status"));
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
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- HTTP WiFi scan request"));
  #endif
  handleRequest(request);
  _wm->requestAsyncScan(true);

  AsyncWebServerResponse *response = request->beginResponse(202, "application/json",
    "{\"accepted\":true,\"state\":\"queued\"}");
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

void WiFiManagerHandlers::handleUpdate(AsyncWebServerRequest *request) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- Handle update"));
  #endif
  if (captivePortal(request)) return;
  handleRequest(request);
  request->redirect(String(F("/#/update")));
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
    
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[OTA] Update file: "), filename.c_str());
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
      #ifndef WM_NO_LOG
      _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] OTA Update ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.begin failed");
      _wm->_configPortalTimeout = _configPortalTimeoutSAV;
      timeoutSaved = false;
      return;
    }
  }
  
  if (len) {
    if (Update.write(data, len) != len) {
      #ifndef WM_NO_LOG
      _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] OTA Update WRITE ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.write failed");
      _wm->_configPortalTimeout = _configPortalTimeoutSAV;
      timeoutSaved = false;
      return;
    }
  }
  
  if (final) {
    if (Update.end(true)) {
      #ifndef WM_NO_LOG
      _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("\n\n[OTA] OTA FILE END bytes: "), (String)index);
      #endif
    } else {
      #ifndef WM_NO_LOG
      _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] OTA Update END ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.end failed");
    }
    
    _wm->_configPortalTimeout = _configPortalTimeoutSAV;
    timeoutSaved = false;
  }
}

void WiFiManagerHandlers::handleUpdateDone(AsyncWebServerRequest *request) {
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- Handle update done"));
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

void WiFiManagerHandlers::sendApiJson(AsyncWebServerRequest *request, int code, const String& json) {
  AsyncWebServerResponse *response = request->beginResponse(code, F("application/json"), json);
  response->addHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

String WiFiManagerHandlers::buildPortalBootstrapJson() {
  std::vector<const WiFiManager::WiFiScanNetwork *> networks;
  collectVisibleScanResults(networks);
  const char *stateStr = "idle";
  switch (_wm->_scan.state) {
    case WiFiManager::WM_SCAN_IDLE:
      stateStr = "idle";
      break;
    case WiFiManager::WM_SCAN_QUEUED:
      stateStr = "queued";
      break;
    case WiFiManager::WM_SCAN_RUNNING:
      stateStr = "running";
      break;
    case WiFiManager::WM_SCAN_COMPLETE:
      stateStr = "complete";
      break;
    case WiFiManager::WM_SCAN_FAILED:
      stateStr = "failed";
      break;
    case WiFiManager::WM_SCAN_TIMEOUT:
      stateStr = "timeout";
      break;
    default:
      stateStr = "idle";
      break;
  }
  String json;
  reservePage(json, 512);
  json += F("{");
  json += F("\"title\":\"");
  jsonAppendEscaped(json, _wm->_title);
  json += F("\",\"subtitle\":\"");
  {
    String sub;
    if (_wm->configPortalActive) {
      sub = _wm->_apName;
    } else {
      sub = _wm->getWiFiHostname() + " - " + WiFi.localIP().toString();
    }
    jsonAppendEscaped(json, sub);
  }
  json += F("\",\"portalActive\":");
  json += _wm->configPortalActive ? F("true") : F("false");
  json += F(",\"features\":{");
  json += F("\"showInfo\":true,\"showUpdate\":");
  json += _wm->_showInfoUpdate ? F("true") : F("false");
  json += F(",\"showErase\":");
  json += _wm->_showInfoErase ? F("true") : F("false");
  json += F(",\"paramsInWifi\":");
  json += _wm->_paramsInWifi ? F("true") : F("false");
  json += F("},\"showBack\":");
  json += _wm->_showBack ? F("true") : F("false");
  json += F(",\"scan\":{\"state\":\"");
  json += stateStr;
  json += F("\",\"count\":");
  json += String((unsigned int)networks.size());
  json += F("}}");
  return json;
}

void WiFiManagerHandlers::buildPlainStatusSummary(String &out) {
  out = "";
  if (_wm->WiFi_SSID() != "") {
    if (WiFi.status() == WL_CONNECTED) {
      out += F("Connected to ");
      out += _wm->WiFi_SSID();
      out += F(" — IP ");
      out += WiFi.localIP().toString();
    } else {
      out += F("Not connected to ");
      out += _wm->WiFi_SSID();
      if (_wm->_lastconxresult == _wm->WL_STATION_WRONG_PASSWORD) {
        out += F(" (wrong password)");
      } else if (_wm->_lastconxresult == WL_NO_SSID_AVAIL) {
        out += F(" (network not found)");
      } else if (_wm->_lastconxresult == WL_CONNECT_FAILED || _wm->_lastconxresult == WL_CONNECTION_LOST) {
        out += F(" (connection failed)");
      }
    }
  } else {
    out += F("No station network configured");
  }
}

void WiFiManagerHandlers::handleApiBootstrap(AsyncWebServerRequest *request) {
  handleRequest(request);
  sendApiJson(request, 200, buildPortalBootstrapJson());
}

void WiFiManagerHandlers::handleApiWifiScanStatus(AsyncWebServerRequest *request) {
  handleWiFiScanStatus(request);
}

void WiFiManagerHandlers::handleApiWifiScan(AsyncWebServerRequest *request) {
  handleWiFiScanRequest(request);
}

void WiFiManagerHandlers::handleApiWifiMeta(AsyncWebServerRequest *request) {
  handleRequest(request);
  String ssidPlaceholder = _wm->WiFi_SSID();
  String passwordPlaceholder = "";
  if (_wm->_showPassword) {
    passwordPlaceholder = _wm->WiFi_psk();
  } else if (_wm->WiFi_psk() != "") {
    passwordPlaceholder = F("********");
  }
  String staticHtml = getStaticOut();
  String paramsHtml;
  if (_wm->_paramsInWifi && _wm->_paramsCount > 0) {
    paramsHtml = renderSectionBreak(getParamOut());
  }
  String json = F("{\"ssidPlaceholder\":\"");
  jsonAppendEscaped(json, ssidPlaceholder);
  json += F("\",\"passwordPlaceholder\":\"");
  jsonAppendEscaped(json, passwordPlaceholder);
  json += F("\",\"staticFieldsHtml\":\"");
  jsonAppendEscaped(json, staticHtml);
  json += F("\",\"paramsHtml\":\"");
  jsonAppendEscaped(json, paramsHtml);
  json += F("\"}");
  sendApiJson(request, 200, json);
}

void WiFiManagerHandlers::handleApiWifiSave(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- API WiFi save"));
#endif
  handleRequest(request);
  applyWifiAndParamsFromRequest(request);
  String json = F("{\"ok\":true,\"message\":\"");
  if (_wm->_ssid == "") {
    json += F("Settings saved");
  } else {
    json += F("Credentials saved");
  }
  json += F("\",\"next\":{\"connectScheduled\":true}}");
  sendApiJson(request, 200, json);
  _wm->connect = true;
}

void WiFiManagerHandlers::handleApiParamsGet(AsyncWebServerRequest *request) {
  handleRequest(request);
  String fields = getParamOut();
  String json = F("{\"paramsHtml\":\"");
  jsonAppendEscaped(json, fields);
  json += F("\"}");
  sendApiJson(request, 200, json);
}

void WiFiManagerHandlers::handleApiParamsSave(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- API params save"));
#endif
  handleRequest(request);
  WiFiManager::WiFiManagerRequestArgs requestArgs(request);
  doParamSave(requestArgs);
  sendApiJson(request, 200, F("{\"ok\":true,\"message\":\"Setup saved\"}"));
}

void WiFiManagerHandlers::handleApiInfo(AsyncWebServerRequest *request) {
  handleRequest(request);
  String summary;
  buildPlainStatusSummary(summary);
  String json = F("{\"status\":[{\"key\":\"summary\",\"label\":\"Status\",\"value\":\"");
  jsonAppendEscaped(json, summary);
  json += F("\"}],\"device\":[");
  bool first = true;
#ifdef ESP8266
  static const char *const deviceIds[] = {"uptime",    "chipid",   "fchipid", "idesize", "flashsize",
                                          "corever",   "bootver",  "cpufreq", "freeheap", "memsketch",
                                          "memsmeter", "lastreset"};
#elif defined(ESP32)
  static const char *const deviceIds[] = {"uptime",   "chipid",   "chiprev", "idesize",  "flashsize",
                                          "cpufreq", "freeheap", "memsketch", "memsmeter", "lastreset", "temp"};
#endif
  appendJsonInfoArrayFromIds(this, json, deviceIds, sizeof(deviceIds) / sizeof(deviceIds[0]), first);
  json += F("],\"wifi\":[");
  first = true;
#ifdef ESP8266
  static const char *const wifiIds[] = {"conx",   "stassid", "staip", "stagw", "stasub", "dnss", "host",
                                        "stamac", "autoconx", "apssid", "apip", "apbssid", "apmac"};
#elif defined(ESP32)
  static const char *const wifiIds[] = {"conx",   "stassid", "staip", "stagw", "stasub", "dnss", "host",
                                        "stamac", "apssid", "apip", "apmac", "aphost", "apbssid"};
#endif
  appendJsonInfoArrayFromIds(this, json, wifiIds, sizeof(wifiIds) / sizeof(wifiIds[0]), first);
  json += F("],\"about\":[");
  first = true;
  static const char *const aboutIds[] = {"aboutver", "aboutarduinover", "aboutsdkver", "aboutdate"};
  appendJsonInfoArrayFromIds(this, json, aboutIds, sizeof(aboutIds) / sizeof(aboutIds[0]), first);
  json += F("],\"actions\":{");
  json += F("\"showUpdate\":");
  json += _wm->_showInfoUpdate ? F("true") : F("false");
  json += F(",\"showErase\":");
  json += _wm->_showInfoErase ? F("true") : F("false");
  json += F(",\"showBack\":");
  json += _wm->_showBack ? F("true") : F("false");
  json += F("}}");
  sendApiJson(request, 200, json);
}

void WiFiManagerHandlers::handleApiStatus(AsyncWebServerRequest *request) {
  handleRequest(request);
  String summary;
  buildPlainStatusSummary(summary);
  String json = F("{\"text\":\"");
  jsonAppendEscaped(json, summary);
  json += F("\"}");
  sendApiJson(request, 200, json);
}

void WiFiManagerHandlers::handleApiDeviceRestart(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("API device restart"));
#endif
  handleRequest(request);
  sendApiJson(request, 200, F("{\"ok\":true,\"message\":\"Restart scheduled\"}"));
  _wm->_rebootScheduled = true;
  _wm->_rebootTime = millis() + _wm->REBOOT_DELAY_MS;
}

void WiFiManagerHandlers::handleApiDeviceErase(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("API device erase"));
#endif
  handleRequest(request);
  bool ret = _wm->erase(false);
  String json = F("{\"ok\":");
  json += ret ? F("true") : F("false");
  json += F(",\"message\":\"");
  if (ret) {
    json += F("WiFi configuration erased. Device will restart shortly.");
  } else {
    json += F("Erase failed");
  }
  json += F("\"}");
  sendApiJson(request, ret ? 200 : 500, json);
  if (ret) {
    _wm->_rebootScheduled = true;
    _wm->_rebootTime = millis() + _wm->ERASE_REBOOT_DELAY_MS;
  }
}

void WiFiManagerHandlers::handleApiPortalClose(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("API portal close"));
#endif
  stopCaptivePortal();
  handleRequest(request);
  sendApiJson(request, 200, F("{\"ok\":true,\"message\":\"Captive portal detection disabled\"}"));
}

void WiFiManagerHandlers::handleApiPortalExit(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("API portal exit"));
#endif
  handleRequest(request);
  sendApiJson(request, 200, F("{\"ok\":true,\"message\":\"Exiting portal\"}"));
  _wm->_abortScheduled = true;
  _wm->_abortTime = millis() + _wm->EXIT_DELAY_MS;
}

#endif // defined(ESP8266) || defined(ESP32)

