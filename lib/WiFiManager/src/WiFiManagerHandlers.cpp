/**
 * WiFiManagerHandlers.cpp
 * 
 * HTTP request handlers and rendering logic implementation
 * 
 * @author alexhopeoconnor
 * @license MIT
 */

#include "WiFiManagerHandlers.h"
#include "templates/CSS.h"
#include "templates/RootShell.h"
#include "templates/PortalAppJS.h"
#include <TemplateEngine.h>
#include <TemplateEngineAsyncWeb.h>
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

static void jsonAppendEscaped(String& out, const WiFiManagerPortalText& text) {
  for (size_t i = 0; i < text.length(); ++i) {
    const char c = text.at(i);
    if (c == '"') {
      out += F("\\\"");
    } else if (c == '\\') {
      out += F("\\\\");
    } else if (c == '\n') {
      out += F("\\n");
    } else if (c != '\r') {
      out += c;
    }
  }
}

namespace {

const char kEmptyPortalPlaceholder[] PROGMEM = "";

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

void configureDynamicStringDescriptor(DynamicDataDescriptor& descriptor, String& value) {
  descriptor.getter = &dynamicStringGetter;
  descriptor.getLength = &dynamicStringLengthGetter;
  descriptor.userData = &value;
}

const char* portalTextGetter(void* userData) {
  const auto* text = static_cast<const WiFiManagerPortalText*>(userData);
  return text && text->storage == WiFiManagerPortalStorage::Ram && text->data
    ? text->data
    : "";
}

size_t portalTextLengthGetter(const char* /*data*/, void* userData) {
  const auto* text = static_cast<const WiFiManagerPortalText*>(userData);
  return text ? text->length() : 0;
}

void configurePortalTextDescriptor(DynamicDataDescriptor& descriptor,
                                  const WiFiManagerPortalText& text) {
  descriptor.getter = &portalTextGetter;
  descriptor.getLength = &portalTextLengthGetter;
  descriptor.userData = const_cast<WiFiManagerPortalText*>(&text);
}


const char* portalHomeCardKindJson(PortalHomeCardKind k) {
  switch (k) {
    case PortalHomeCardKind::Text:
      return "text";
    case PortalHomeCardKind::Callout:
      return "callout";
    case PortalHomeCardKind::KeyValue:
    default:
      return "kv";
  }
}

template <typename BundleT>
AsyncWebServerResponse* beginTemplateResponse(AsyncWebServerRequest* request,
                                             const std::shared_ptr<BundleT>& bundle,
                                             const char* templateData) {
  bundle->context.setRegistry(&bundle->registry);
  TemplateRenderer::initializeContext(bundle->context, templateData);
  return TemplateEngineAsyncWeb::beginSafeChunkedResponse(
    request,
    String(FPSTR(HTTP_HEAD_CT)),
    bundle,
    [](BundleT& responseBundle, uint8_t *buffer, size_t maxLen, size_t /*index*/) -> size_t {
      return TemplateEngineAsyncWeb::renderTemplateChunkWithRetries(
        responseBundle.context, buffer, maxLen, 128);
    },
    [](const BundleT& responseBundle) -> bool {
      return TemplateEngineAsyncWeb::isTemplateTerminal(responseBundle.context);
    }
  );
}

// Shell contract (must match templates/RootShell.h):
//   %PAGE_TITLE%        -> document title (from WiFiManager title state)
//   %STYLES%            -> immutable built-in portal stylesheet in PROGMEM
//   %PORTAL_THEME%      -> optional validated semantic-token override
//   %BOOTSTRAP_JSON%    -> initial SPA runtime payload
//   %PORTAL_APP_JS%     -> embedded SPA source

struct PortalShellRenderBundle {
  String bootstrapJson;
  WiFiManagerPortalText pageTitle;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicDataDescriptor bootstrapDescriptor;
  DynamicDataDescriptor pageTitleDescriptor;
  DynamicDataDescriptor themeDescriptor;

  PortalShellRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        bootstrapDescriptor{},
        pageTitleDescriptor{},
        themeDescriptor{} {}
};

// Custom HTML attribute strings (e.g. from DeviceFramework generateCustomHTML) may include
// type='password' — the JSON field descriptor must echo that or the SPA renders type=text.
bool portalCustomAttrsIndicatePassword(const String& customAttrs) {
  if (customAttrs.length() == 0) {
    return false;
  }
  return customAttrs.indexOf("type='password'") >= 0 || customAttrs.indexOf("type=\"password\"") >= 0;
}

}  // namespace

WiFiManagerHandlers::WiFiManagerHandlers(WiFiManager* wm) : _wm(wm) {}

void WiFiManagerHandlers::appendPortalExtraInfoSectionsJson(String& json, bool& first) {
  if (_wm == nullptr) {
    return;
  }
  for (const auto& sec : _wm->_portalStructured.infoSections) {
    if (!first) {
      json += F(",");
    }
    first = false;
    json += F("{\"id\":\"");
    jsonAppendEscaped(json, sec.id);
    json += F("\",\"title\":\"");
    jsonAppendEscaped(json, sec.title);
    json += F("\",\"items\":[");
    bool firstItem = true;
    for (const auto& it : sec.items) {
      if (!firstItem) {
        json += F(",");
      }
      firstItem = false;
      json += F("{\"key\":\"");
      jsonAppendEscaped(json, it.key);
      json += F("\",\"label\":\"");
      jsonAppendEscaped(json, it.label);
      json += F("\",\"value\":\"");
      jsonAppendEscaped(json, it.value);
      json += F("\"}");
    }
    json += F("]}");
  }
}
void WiFiManagerHandlers::appendPortalExtraHomeCardsJson(String& json, bool& first) {
  if (_wm == nullptr) {
    return;
  }
  for (const auto& card : _wm->_portalStructured.homeCards) {
    if (!first) {
      json += F(",");
    }
    first = false;
    json += F("{\"id\":\"");
    jsonAppendEscaped(json, card.id);
    json += F("\",\"title\":\"");
    jsonAppendEscaped(json, card.title);
    json += F("\",\"kind\":\"");
    json += portalHomeCardKindJson(card.kind);
    json += F("\",\"text\":\"");
    jsonAppendEscaped(json, card.text);
    json += F("\",\"items\":[");
    bool firstItem = true;
    for (const auto& it : card.items) {
      if (!firstItem) {
        json += F(",");
      }
      firstItem = false;
      json += F("{\"key\":\"");
      jsonAppendEscaped(json, it.key);
      json += F("\",\"label\":\"");
      jsonAppendEscaped(json, it.label);
      json += F("\",\"value\":\"");
      jsonAppendEscaped(json, it.value);
      json += F("\"}");
    }
    json += F("]}");
  }
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

void WiFiManagerHandlers::appendPortalJsonStaticFields(String& json, bool& first) {
  auto emitField = [&](const char* fieldId, const __FlashStringHelper* label, const String& val) {
    if (!first) {
      json += F(",");
    }
    first = false;
    json += F("{\"id\":\"");
    json += fieldId;
    json += F("\",\"name\":\"");
    json += fieldId;
    json += F("\",\"label\":\"");
    json += label;
    json += F("\",\"type\":\"text\",\"maxlength\":15,\"value\":\"");
    jsonAppendEscaped(json, val);
    json += F("\"}");
  };

  if ((_wm->_staShowStaticFields || _wm->_sta_static_ip) && _wm->_staShowStaticFields >= 0) {
    emitField("ip", F("Static IP"), (_wm->_sta_static_ip ? _wm->_sta_static_ip.toString() : ""));
    emitField("gw", F("Static gateway"), (_wm->_sta_static_gw ? _wm->_sta_static_gw.toString() : ""));
    emitField("sn", F("Subnet"), (_wm->_sta_static_sn ? _wm->_sta_static_sn.toString() : ""));
  }
  if ((_wm->_staShowDns || _wm->_sta_static_dns) && _wm->_staShowDns >= 0) {
    emitField("dns", F("Static DNS"), (_wm->_sta_static_dns ? _wm->_sta_static_dns.toString() : ""));
  }
}

void WiFiManagerHandlers::appendPortalJsonCustomParams(String& json, bool& first) {
  const int n = _wm->getParametersCount();
  if (n <= 0) {
    return;
  }
  WiFiManagerParameter** params = _wm->getParameters();
  for (int i = 0; i < n; i++) {
    WiFiManagerParameter* p = params[i];
    if (p == NULL || p->getValueLength() > 99999) {
      continue;
    }
    if (!first) {
      json += F(",");
    }
    first = false;
    if (p->getID() != nullptr) {
      String pid = String(p->getID());
      const String customAttrs = String(p->getCustomHTML());
      json += F("{\"kind\":\"field\",\"name\":\"");
      jsonAppendEscaped(json, pid);
      json += F("\",\"id\":\"");
      jsonAppendEscaped(json, pid);
      json += F("\",\"label\":\"");
      jsonAppendEscaped(json, String(p->getLabel()));
      json += F("\",\"type\":\"");
      json += portalCustomAttrsIndicatePassword(customAttrs) ? F("password") : F("text");
      json += F("\",\"maxlength\":");
      json += String(p->getValueLength());
      json += F(",\"value\":\"");
      jsonAppendEscaped(json, String(p->getValue()));
      json += F("\",\"labelPlacement\":");
      json += String(p->getLabelPlacement());
      if (customAttrs.length() > 0) {
        json += F(",\"customAttrs\":\"");
        jsonAppendEscaped(json, customAttrs);
        json += F("\"");
      }
      json += F("}");
    } else {
      json += F("{\"kind\":\"html\",\"html\":\"");
      jsonAppendEscaped(json, String(p->getCustomHTML()));
      json += F("\"}");
    }
  }
}

void WiFiManagerHandlers::appendJsonKvItem(String& json, bool& first, const char* key, const String& label,
                                           const String& value) {
  if (!first) {
    json += F(",");
  }
  first = false;
  json += F("{\"key\":\"");
  json += key;
  json += F("\",\"label\":\"");
  jsonAppendEscaped(json, label);
  json += F("\",\"value\":\"");
  jsonAppendEscaped(json, value);
  json += F("\"}");
}

void WiFiManagerHandlers::appendInfoSectionFromIds(String& json, const char* const* ids, size_t count, bool& first) {
  for (size_t i = 0; i < count; i++) {
    appendOneInfoItemForId(json, first, ids[i]);
  }
}

void WiFiManagerHandlers::appendOneInfoItemForId(String& json, bool& first, const char* id) {
  if (strcmp(id, "uptime") == 0) {
    appendJsonKvItem(json, first, id, String(F("Uptime")),
                     String(millis() / 1000 / 60) + F(" mins ") + String((millis() / 1000) % 60) + F(" secs"));
    return;
  }
  if (strcmp(id, "chipid") == 0) {
#ifdef ESP8266
    appendJsonKvItem(json, first, id, String(F("Chip ID")), String(ESP.getChipId(), HEX));
#elif defined(ESP32)
    appendJsonKvItem(json, first, id, String(F("Chip ID")), String((uint32_t)ESP.getEfuseMac(), HEX));
#endif
    return;
  }
#ifdef ESP32
  if (strcmp(id, "chiprev") == 0) {
    String rev = (String)ESP.getChipRevision();
#ifdef _SOC_EFUSE_REG_H_
    String revb =
        (String)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S)&EFUSE_RD_CHIP_VER_RESERVE_V);
    appendJsonKvItem(json, first, id, String(F("Chip rev")), rev + F(" / ") + revb);
#else
    appendJsonKvItem(json, first, id, String(F("Chip rev")), rev);
#endif
    return;
  }
#endif
#ifdef ESP8266
  if (strcmp(id, "fchipid") == 0) {
    appendJsonKvItem(json, first, id, String(F("Flash chip ID")), String(ESP.getFlashChipId()));
    return;
  }
#endif
  if (strcmp(id, "idesize") == 0) {
    appendJsonKvItem(json, first, id, String(F("Flash size")), String(ESP.getFlashChipSize()) + F(" bytes"));
    return;
  }
  if (strcmp(id, "flashsize") == 0) {
#ifdef ESP8266
    appendJsonKvItem(json, first, id, String(F("Real flash size")), String(ESP.getFlashChipRealSize()) + F(" bytes"));
#elif defined(ESP32)
    appendJsonKvItem(json, first, id, String(F("PSRAM Size")), String(ESP.getPsramSize()) + F(" bytes"));
#endif
    return;
  }
  if (strcmp(id, "corever") == 0) {
#ifdef ESP8266
    appendJsonKvItem(json, first, id, String(F("Core version")), String(ESP.getCoreVersion()));
#endif
    return;
  }
#ifdef ESP8266
  if (strcmp(id, "bootver") == 0) {
    appendJsonKvItem(json, first, id, String(F("Boot version")), String(system_get_boot_version()));
    return;
  }
#endif
  if (strcmp(id, "cpufreq") == 0) {
    appendJsonKvItem(json, first, id, String(F("CPU frequency")), String(ESP.getCpuFreqMHz()) + F("MHz"));
    return;
  }
  if (strcmp(id, "freeheap") == 0) {
    appendJsonKvItem(json, first, id, String(F("Memory - Free heap")), String(ESP.getFreeHeap()) + F(" bytes available"));
    return;
  }
  if (strcmp(id, "memsketch") == 0) {
    appendJsonKvItem(json, first, id, String(F("Memory - Sketch size")),
                     String(ESP.getSketchSize()) + F(" / ") + String(ESP.getSketchSize() + ESP.getFreeSketchSpace()) +
                         F(" bytes"));
    return;
  }
  if (strcmp(id, "memsmeter") == 0) {
    appendJsonKvItem(json, first, id, String(F("Sketch usage (used / max)")),
                     String(ESP.getSketchSize()) + F(" / ") + String(ESP.getSketchSize() + ESP.getFreeSketchSpace()));
    return;
  }
  if (strcmp(id, "lastreset") == 0) {
#ifdef ESP8266
    appendJsonKvItem(json, first, id, String(F("Last reset reason")), String(ESP.getResetReason()));
#elif defined(ESP32) && defined(_ROM_RTC_H_)
    String reasons[2];
    for (int i = 0; i < 2; i++) {
      int reason = rtc_get_reset_reason(i);
      switch (reason) {
        case 1:
          reasons[i] = F("Vbat power on reset");
          break;
        case 3:
          reasons[i] = F("Software reset digital core");
          break;
        case 4:
          reasons[i] = F("Legacy watch dog reset digital core");
          break;
        case 5:
          reasons[i] = F("Deep Sleep reset digital core");
          break;
        case 6:
          reasons[i] = F("Reset by SLC module, reset digital core");
          break;
        case 7:
          reasons[i] = F("Timer Group0 Watch dog reset digital core");
          break;
        case 8:
          reasons[i] = F("Timer Group1 Watch dog reset digital core");
          break;
        case 9:
          reasons[i] = F("RTC Watch dog Reset digital core");
          break;
        case 10:
          reasons[i] = F("Instrusion tested to reset CPU");
          break;
        case 11:
          reasons[i] = F("Time Group reset CPU");
          break;
        case 12:
          reasons[i] = F("Software reset CPU");
          break;
        case 13:
          reasons[i] = F("RTC Watch dog Reset CPU");
          break;
        case 14:
          reasons[i] = F("for APP CPU, reseted by PRO CPU");
          break;
        case 15:
          reasons[i] = F("Reset when the vdd voltage is not stable");
          break;
        case 16:
          reasons[i] = F("RTC Watch dog reset digital core and rtc module");
          break;
        default:
          reasons[i] = F("NO_MEAN");
      }
    }
    appendJsonKvItem(json, first, id, String(F("Last reset reason")),
                     String(F("CPU0: ")) + reasons[0] + F(" — CPU1: ") + reasons[1]);
#endif
    return;
  }
  if (strcmp(id, "apip") == 0) {
    appendJsonKvItem(json, first, id, String(F("Access point IP")), WiFi.softAPIP().toString());
    return;
  }
  if (strcmp(id, "apmac") == 0) {
    appendJsonKvItem(json, first, id, String(F("Access point MAC")), WiFi.softAPmacAddress());
    return;
  }
#ifdef ESP32
  if (strcmp(id, "aphost") == 0) {
    appendJsonKvItem(json, first, id, String(F("Access point hostname")), String(WiFi.softAPgetHostname()));
    return;
  }
#endif
#ifndef WM_NOSOFTAPSSID
#ifdef ESP8266
  if (strcmp(id, "apssid") == 0) {
    appendJsonKvItem(json, first, id, String(F("Access point SSID")), WiFi.softAPSSID());
    return;
  }
#endif
#endif
  if (strcmp(id, "apbssid") == 0) {
    appendJsonKvItem(json, first, id, String(F("BSSID")), WiFi.BSSIDstr());
    return;
  }
  if (strcmp(id, "stassid") == 0) {
    appendJsonKvItem(json, first, id, String(F("Station SSID")), (String)_wm->WiFi_SSID());
    return;
  }
  if (strcmp(id, "staip") == 0) {
    appendJsonKvItem(json, first, id, String(F("Station IP")), WiFi.localIP().toString());
    return;
  }
  if (strcmp(id, "stagw") == 0) {
    appendJsonKvItem(json, first, id, String(F("Station gateway")), WiFi.gatewayIP().toString());
    return;
  }
  if (strcmp(id, "stasub") == 0) {
    appendJsonKvItem(json, first, id, String(F("Station subnet")), WiFi.subnetMask().toString());
    return;
  }
  if (strcmp(id, "dnss") == 0) {
    appendJsonKvItem(json, first, id, String(F("DNS Server")), WiFi.dnsIP().toString());
    return;
  }
  if (strcmp(id, "host") == 0) {
#ifdef ESP32
    appendJsonKvItem(json, first, id, String(F("Hostname")), String(WiFi.getHostname()));
#else
    appendJsonKvItem(json, first, id, String(F("Hostname")), String(WiFi.hostname()));
#endif
    return;
  }
  if (strcmp(id, "stamac") == 0) {
    appendJsonKvItem(json, first, id, String(F("Station MAC")), WiFi.macAddress());
    return;
  }
  if (strcmp(id, "conx") == 0) {
    appendJsonKvItem(json, first, id, String(F("Connected")), WiFi.isConnected() ? String(F("Yes")) : String(F("No")));
    return;
  }
#ifdef ESP8266
  if (strcmp(id, "autoconx") == 0) {
    appendJsonKvItem(json, first, id, String(F("Autoconnect")),
                     WiFi.getAutoConnect() ? String(F("Enabled")) : String(F("Disabled")));
    return;
  }
#endif
#if defined(ESP32) && !defined(WM_NOTEMP)
  if (strcmp(id, "temp") == 0) {
    appendJsonKvItem(json, first, id, String(F("Temperature")),
                     String(temperatureRead()) + F(" C / ") + String((temperatureRead() + 32) * 1.8f) + F(" F"));
    return;
  }
#endif
  if (strcmp(id, "aboutver") == 0) {
    appendJsonKvItem(json, first, id, String(F("WiFiManager")), String(FPSTR(WM_VERSION_STR)));
    return;
  }
  if (strcmp(id, "aboutarduinover") == 0) {
#ifdef VER_ARDUINO_STR
    appendJsonKvItem(json, first, id, String(F("Arduino")), String(VER_ARDUINO_STR));
#endif
    return;
  }
  if (strcmp(id, "aboutsdkver") == 0) {
#ifdef ESP32
    appendJsonKvItem(json, first, id, String(F("SDK version")), String(esp_get_idf_version()));
#else
    appendJsonKvItem(json, first, id, String(F("SDK version")), String(system_get_sdk_version()));
#endif
    return;
  }
  if (strcmp(id, "aboutdate") == 0) {
    appendJsonKvItem(json, first, id, String(F("Build date")), String(__DATE__ " " __TIME__));
    return;
  }
}


// Captive Portal

bool WiFiManagerHandlers::shouldRedirectCaptiveForHost(const String& requestHost, const String& serverLocWithPort) {
  return serverLocWithPort.length() > 0 && requestHost != serverLocWithPort;
}

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
  bool doredirect = shouldRedirectCaptiveForHost(request->host(), serverLoc);
  
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

  // Root shell render model:
  // - WiFiManagerHandlers owns request-time shell assembly; WiFiManagerServer owns HTTP lifecycle only.
  // - Build one request-scoped PortalShellRenderBundle.
  // - Populate one request-scoped PlaceholderRegistry with shell defaults + request payloads.
  // - Render WM_ROOT_SHELL_TEMPLATE with static CSS plus an optional small theme block.
  auto bundle = std::make_shared<PortalShellRenderBundle>();
  bundle->pageTitle = _wm ? _wm->_portalBrand.title : WiFiManagerPortalText{};
  bundle->bootstrapJson = buildPortalBootstrapJson();
  bundle->registry.registerProgmemData("%STYLES%", CSS_STYLE);
  bundle->registry.registerProgmemData("%PORTAL_APP_JS%", PORTAL_APP_JS);
  if (_wm) {
    configureDynamicStringDescriptor(bundle->themeDescriptor, _wm->_portalThemeStyle);
    bundle->registry.registerDynamicData("%PORTAL_THEME%", &bundle->themeDescriptor);
  } else {
    bundle->registry.registerProgmemData("%PORTAL_THEME%", kEmptyPortalPlaceholder);
  }
  if (bundle->pageTitle.storage == WiFiManagerPortalStorage::Progmem && bundle->pageTitle.data) {
    bundle->registry.registerProgmemData("%PAGE_TITLE%", bundle->pageTitle.data);
  } else {
    configurePortalTextDescriptor(bundle->pageTitleDescriptor, bundle->pageTitle);
    bundle->registry.registerDynamicData("%PAGE_TITLE%", &bundle->pageTitleDescriptor);
  }
  configureDynamicStringDescriptor(bundle->bootstrapDescriptor, bundle->bootstrapJson);
  bundle->registry.registerDynamicData("%BOOTSTRAP_JSON%", &bundle->bootstrapDescriptor);

  request->send(beginTemplateResponse(request, bundle, WM_ROOT_SHELL_TEMPLATE));
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
    requestinfo += " " + p->name() + ": [redacted, " + String(p->value().length()) + " chars]\n";
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

  if (_wm->_portalLayout.paramsOnWifiPage) {
    doParamSave(requestArgs);
  }
}

bool WiFiManagerHandlers::buildStationProfilesFromRequest(
    AsyncWebServerRequest *request, WiFiManagerStationProfiles& profiles) {
  profiles = _wm->_stationProfiles;
  if (!_wm->validateStationProfiles(profiles)) {
    profiles = WiFiManagerStationProfiles();
  }
  profiles.preferredSlot = 0;

  for (uint8_t slot = 0; slot < WM_STATION_PROFILE_COUNT; ++slot) {
    WiFiManagerStationProfile& profile = profiles.slots[slot];
    const String ssidName = String(F("s")) + String(slot);
    const String passwordName = String(F("p")) + String(slot);
    const String clearName = String(F("clear")) + String(slot);

    if (request->hasParam(ssidName.c_str(), true)) {
      const String ssid = request->getParam(ssidName.c_str(), true)->value();
      if (ssid.length() >= sizeof(profile.ssid)) {
        return false;
      }
      memset(profile.ssid, 0, sizeof(profile.ssid));
      memcpy(profile.ssid, ssid.c_str(), ssid.length());
      profile.enabled = ssid.length() > 0;
      if (!profile.enabled) {
        profile.hasPassword = false;
        memset(profile.password, 0, sizeof(profile.password));
        if (profiles.lastSuccessfulSlot == slot) {
          profiles.lastSuccessfulSlot = WM_NO_STATION_PROFILE;
        }
      }
    }

    if (request->hasParam(clearName.c_str(), true)) {
      profile.hasPassword = false;
      memset(profile.password, 0, sizeof(profile.password));
    } else if (request->hasParam(passwordName.c_str(), true)) {
      const String password = request->getParam(passwordName.c_str(), true)->value();
      if (password.length() >= sizeof(profile.password)) {
        return false;
      }
      // A blank password means "unchanged". Explicit clear is used for an
      // open network so a browser never erases a stored secret by accident.
      if (password.length() > 0) {
        memset(profile.password, 0, sizeof(profile.password));
        memcpy(profile.password, password.c_str(), password.length());
        profile.hasPassword = true;
      }
    }
  }

  if (profiles.lastSuccessfulSlot != WM_NO_STATION_PROFILE &&
      !_wm->isStationProfileEnabled(profiles, profiles.lastSuccessfulSlot)) {
    profiles.lastSuccessfulSlot = WM_NO_STATION_PROFILE;
  }
  return _wm->validateStationProfiles(profiles);
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
      _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,
          (String)_wm->_params[i]->getID() + F(": [redacted, ") + String(value.length()) + F(" chars]"));
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

void WiFiManagerHandlers::appendVisibleScanResultsJson(String& json, const std::vector<const WiFiManager::WiFiScanNetwork*>& networks) {
  for (size_t i = 0; i < networks.size(); i++) {
    const auto* network = networks[i];
    if (i > 0) {
      json += ",";
    }
    json += "{";
    json += "\"ssid\":\"";
    jsonAppendEscaped(json, network->ssid);
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
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- Handle update done (POST /u)"));
#endif
  handleRequest(request);

  if (Update.hasError()) {
    String json = F("{\"ok\":false,\"message\":\"Update failed\",\"detail\":\"");
#ifdef ESP32
    jsonAppendEscaped(json, String(Update.errorString()));
#else
    jsonAppendEscaped(json, String(Update.getError()));
#endif
    json += F("\"}");
    sendApiJson(request, 500, json);
    return;
  }

  sendApiJson(request, 200, jsonApiOtaUpdateSuccess());
  delay(1000);
  ESP.restart();
}

void WiFiManagerHandlers::sendApiJson(AsyncWebServerRequest *request, int code, const String& json) {
  AsyncWebServerResponse *response = request->beginResponse(code, F("application/json"), json);
  response->addHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

void WiFiManagerHandlers::appendApiInfoActionsJson(String& json) {
  const bool portalRunning = _wm->configPortalActive || _wm->webPortalActive;
  json += F("\"showUpdate\":");
  json += _wm->_portalPages.updateVisible ? F("true") : F("false");
  json += F(",\"showErase\":");
  json += _wm->_portalActions.eraseVisible ? F("true") : F("false");
  json += F(",\"showBack\":");
  json += _wm->_portalActions.backVisible ? F("true") : F("false");
  json += F(",\"showRestart\":");
  json += (_wm->_portalActions.restartVisible && portalRunning) ? F("true") : F("false");
  json += F(",\"showExitPortal\":");
  json += (_wm->_portalActions.exitVisible && _wm->_allowExit && portalRunning) ? F("true") : F("false");
  json += F(",\"showCloseCaptive\":");
  json += (_wm->_portalActions.closeCaptiveVisible && _wm->_enableCaptivePortal && _wm->configPortalActive) ? F("true") : F("false");
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

  const bool portalRunning = _wm->configPortalActive || _wm->webPortalActive;
  String json;
  reservePage(json, 1200);
  json += F("{\"contractVersion\":3");
  json += F(",\"brand\":{");
  json += F("\"title\":\"");
  jsonAppendEscaped(json, _wm->_portalBrand.title);
  json += F("\",\"tagline\":\"");
  jsonAppendEscaped(json, _wm->_portalBrand.tagline);
  json += F("\",\"logoSvg\":\"");
  jsonAppendEscaped(json, _wm->_portalBrand.logo.svg);
  json += F("\",\"logoAltText\":\"");
  jsonAppendEscaped(json, _wm->_portalBrand.logoAltText);
  json += F("\"}");
  json += F(",\"context\":{");
  json += F("\"portalActive\":");
  json += portalRunning ? F("true") : F("false");
  json += F(",\"portalTimeoutSecondsRemaining\":");
  {
    unsigned long timeoutRemainingSeconds = 0;
    if (_wm->configPortalActive && _wm->_configPortalTimeout > 0) {
      const unsigned long now = millis();
      const unsigned long timeoutAt = _wm->_configPortalStart + _wm->_configPortalTimeout;
      if (timeoutAt > now) {
        timeoutRemainingSeconds = (timeoutAt - now + 999UL) / 1000UL;
      }
    }
    json += String(timeoutRemainingSeconds);
  }
  json += F(",\"identityText\":\"");
  if (!_wm->_portalBrand.identityTextOverride.empty()) {
    jsonAppendEscaped(json, _wm->_portalBrand.identityTextOverride);
  } else {
    const String idText = _wm->configPortalActive
      ? _wm->_apName
      : _wm->getWiFiHostname() + " - " + WiFi.localIP().toString();
    jsonAppendEscaped(json, idText);
  }
  json += F("\",\"statusSummary\":\"");
  {
    String st;
    buildPlainStatusSummary(st);
    jsonAppendEscaped(json, st);
  }
  json += F("\",\"scan\":{\"state\":\"");
  json += stateStr;
  json += F("\",\"count\":");
  json += String((unsigned int)networks.size());
  json += F("}}");
  json += F(",\"pages\":{");
  json += F("\"wifi\":{\"visible\":true}");
  json += F(",\"setup\":{\"visible\":");
  json += (!_wm->_portalLayout.paramsOnWifiPage && _wm->_portalPages.setupVisible) ? F("true") : F("false");
  json += F("}");
  json += F(",\"info\":{\"visible\":");
  json += _wm->_portalPages.infoVisible ? F("true") : F("false");
  json += F("}");
  json += F(",\"update\":{\"visible\":");
  json += _wm->_portalPages.updateVisible ? F("true") : F("false");
  json += F("}}");
  json += F(",\"actions\":{");
  json += F("\"erase\":{\"visible\":");
  json += _wm->_portalActions.eraseVisible ? F("true") : F("false");
  json += F("},\"restart\":{\"visible\":");
  json += (_wm->_portalActions.restartVisible && portalRunning) ? F("true") : F("false");
  json += F("},\"exitPortal\":{\"visible\":");
  json += (_wm->_portalActions.exitVisible && _wm->_allowExit && portalRunning) ? F("true") : F("false");
  json += F("},\"closeCaptive\":{\"visible\":");
  json += (_wm->_portalActions.closeCaptiveVisible && _wm->_enableCaptivePortal && _wm->configPortalActive) ? F("true") : F("false");
  json += F("},\"back\":{\"visible\":");
  json += _wm->_portalActions.backVisible ? F("true") : F("false");
  json += F("}}");
  json += F(",\"layout\":{\"paramsLocation\":\"");
  json += _wm->_portalLayout.paramsOnWifiPage ? F("wifi") : F("setup");
  json += F("\"}");
  bool first = true;
  json += F(",\"extraHomeCards\":[");
  appendPortalExtraHomeCardsJson(json, first);
  json += F("]}");
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
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- API WiFi scan status"));
#endif
  handleRequest(request);

  std::vector<const WiFiManager::WiFiScanNetwork *> networks;
  collectVisibleScanResults(networks);

  String json = "{";
  reservePage(json, 160 + (networks.size() > 0 ? networks.size() * 96 : 64));
  json += "\"state\":\"";
  switch (_wm->_scan.state) {
    case WiFiManager::WM_SCAN_IDLE:
      json += "idle";
      break;
    case WiFiManager::WM_SCAN_QUEUED:
      json += "queued";
      break;
    case WiFiManager::WM_SCAN_RUNNING:
      json += "running";
      break;
    case WiFiManager::WM_SCAN_COMPLETE:
      json += "complete";
      break;
    case WiFiManager::WM_SCAN_FAILED:
      json += "failed";
      break;
    case WiFiManager::WM_SCAN_TIMEOUT:
      json += "timeout";
      break;
  }
  json += "\",\"scanning\":";
  json += (_wm->_scan.state == WiFiManager::WM_SCAN_RUNNING || _wm->_scan.state == WiFiManager::WM_SCAN_QUEUED)
              ? "true"
              : "false";
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

void WiFiManagerHandlers::handleApiWifiScan(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- API WiFi scan"));
#endif
  handleRequest(request);
  _wm->requestAsyncScan(true);

  AsyncWebServerResponse *response =
      request->beginResponse(202, "application/json", jsonApiWifiScanAccepted());
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

String WiFiManagerHandlers::buildApiWifiMetaJson() {
  if (_wm->isStationProfileMode()) {
    const WiFiManagerStationProfiles& profiles = _wm->getStationProfiles();
    const WiFiManager::wm_station_status_t& station = _wm->getStationStatus();
    String json = F("{\"profiles\":[");
    for (uint8_t slot = 0; slot < WM_STATION_PROFILE_COUNT; ++slot) {
      if (slot > 0) json += ',';
      const WiFiManagerStationProfile& profile = profiles.slots[slot];
      json += F("{\"slot\":\"");
      json += slot == 0 ? F("primary") : F("fallback");
      json += F("\",\"configured\":");
      json += profile.enabled ? F("true") : F("false");
      json += F(",\"ssid\":\"");
      jsonAppendEscaped(json, profile.enabled ? String(profile.ssid) : String());
      json += F("\",\"passwordSet\":");
      json += profile.hasPassword ? F("true") : F("false");
      json += F("}");
    }
    json += F("],\"activeSlot\":");
    if (station.activeSlot == WM_NO_STATION_PROFILE) json += F("null");
    else json += station.activeSlot == 0 ? F("\"primary\"") : F("\"fallback\"");
    json += F(",\"state\":");
    switch (station.state) {
      case WiFiManager::WM_STATION_ATTEMPTING: json += F("\"connecting\""); break;
      case WiFiManager::WM_STATION_CONNECTED: json += F("\"connected\""); break;
      case WiFiManager::WM_STATION_BACKOFF: json += F("\"backoff\""); break;
      case WiFiManager::WM_STATION_PORTAL: json += F("\"portal\""); break;
      default: json += F("\"idle\""); break;
    }
    json += F(",\"wifiFields\":[],\"staticFields\":[");
    bool first = true;
    appendPortalJsonStaticFields(json, first);
    json += F("],\"params\":[");
    first = true;
    if (_wm->_portalLayout.paramsOnWifiPage && _wm->getParametersCount() > 0) {
      appendPortalJsonCustomParams(json, first);
    }
    json += F("],\"actions\":{\"canRefreshScan\":true,\"showBack\":");
    json += _wm->_portalActions.backVisible ? F("true") : F("false");
    json += F("}}");
    return json;
  }

  String ssidPlaceholder = _wm->WiFi_SSID();
  String passwordPlaceholder = "";
  switch (_wm->_portalPasswordPlaceholderMode) {
    case PortalPasswordPlaceholderMode::Actual:
      passwordPlaceholder = _wm->WiFi_psk();
      break;
    case PortalPasswordPlaceholderMode::Masked:
      if (_wm->WiFi_psk() != "") {
        passwordPlaceholder = F("********");
      }
      break;
    case PortalPasswordPlaceholderMode::Hidden:
    default:
      passwordPlaceholder = "";
      break;
  }

  String json = F("{\"wifiFields\":[");
  json += F("{\"id\":\"s\",\"name\":\"s\",\"label\":\"SSID\",\"type\":\"text\",\"maxlength\":32,\"value\":\"");
  jsonAppendEscaped(json, ssidPlaceholder);
  json += F("\"},{\"id\":\"p\",\"name\":\"p\",\"label\":\"Password\",\"type\":\"password\",\"maxlength\":64,\"placeholder\":\"");
  jsonAppendEscaped(json, passwordPlaceholder);
  json += F("\"}],\"staticFields\":[");
  bool first = true;
  appendPortalJsonStaticFields(json, first);
  json += F("],\"params\":[");
  first = true;
  if (_wm->_portalLayout.paramsOnWifiPage && _wm->getParametersCount() > 0) {
    appendPortalJsonCustomParams(json, first);
  }
  json += F("],\"actions\":{\"canRefreshScan\":true,\"showBack\":");
  json += _wm->_portalActions.backVisible ? F("true") : F("false");
  json += F("}}");
  return json;
}

void WiFiManagerHandlers::handleApiWifiMeta(AsyncWebServerRequest *request) {
  handleRequest(request);
  sendApiJson(request, 200, buildApiWifiMetaJson());
}

void WiFiManagerHandlers::handleApiWifiSave(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- WiFi save"));
#endif
  handleRequest(request);
  applyWifiAndParamsFromRequest(request);
  if (_wm->isStationProfileMode()) {
    WiFiManagerStationProfiles candidate;
    if (!buildStationProfilesFromRequest(request, candidate)) {
      sendApiJson(request, 400, F("{\"ok\":false,\"message\":\"Primary WiFi is required and SSID/password lengths must be valid\"}"));
      return;
    }
    const bool saveForLater = request->hasParam("stationAction", true) &&
        request->getParam("stationAction", true)->value() == F("save");
    if (saveForLater) {
      if (!_wm->saveStationProfiles(candidate)) {
        sendApiJson(request, 500, F("{\"ok\":false,\"message\":\"WiFi profiles could not be saved\"}"));
        return;
      }
      sendApiJson(request, 200, F("{\"ok\":true,\"message\":\"WiFi profiles saved for later\"}"));
      return;
    }
    if (!_wm->startStationCandidate(candidate)) {
      sendApiJson(request, 400, F("{\"ok\":false,\"message\":\"WiFi profile candidate was rejected\"}"));
      return;
    }
    sendApiJson(request, 202,
        F("{\"ok\":true,\"message\":\"WiFi profiles accepted\",\"next\":{\"poll\":\"/api/wifi/connect-status\"}}"));
    return;
  }
  _wm->queuePortalConnect(_wm->_ssid, _wm->_pass);
  sendApiJson(
      request, 202,
      F("{\"ok\":true,\"message\":\"Settings accepted\",\"next\":{\"poll\":\"/api/wifi/connect-status\"}}"));
}

String WiFiManagerHandlers::buildApiWifiConnectStatusJson() {
  String json = F("{\"state\":\"");
  switch (_wm->getConfigPortalConnectState()) {
    case WiFiManager::WM_CP_CONNECT_WAITING:
      json += "waiting";
      break;
    case WiFiManager::WM_CP_CONNECT_SUCCESS:
      json += "success";
      break;
    case WiFiManager::WM_CP_CONNECT_FAILED:
      json += "failed";
      break;
    case WiFiManager::WM_CP_CONNECT_IDLE:
    default:
      json += "idle";
      break;
  }
  json += F("\",\"message\":\"");
  jsonAppendEscaped(json, _wm->getConfigPortalConnectMessage());
  json += F("\",\"wifiStatus\":\"");
  jsonAppendEscaped(json, _wm->getWLStatusString(_wm->getConfigPortalConnectStatus()));
  json += F("\"");
  if (_wm->_cpConnectStationIp.length() > 0) {
    json += F(",\"stationIp\":\"");
    jsonAppendEscaped(json, _wm->_cpConnectStationIp);
    json += F("\",\"redirectUrl\":\"http://");
    jsonAppendEscaped(json, _wm->_cpConnectStationIp);
    if (_wm->_httpPort != 80) {
      json += F(":");
      json += String(_wm->_httpPort);
    }
    json += F("/\"");
  }
  json += F("}");
  return json;
}

void WiFiManagerHandlers::handleApiWifiConnectStatus(AsyncWebServerRequest *request) {
  handleRequest(request);
  sendApiJson(request, 200, buildApiWifiConnectStatusJson());
}

void WiFiManagerHandlers::handleApiWifiConnectComplete(AsyncWebServerRequest *request) {
  handleRequest(request);
  if (!_wm->didConfigPortalConnectSucceed() || _wm->_cpConnectStationIp.length() == 0) {
    sendApiJson(request, 409, F("{\"ok\":false,\"message\":\"WiFi hand-off is not ready\"}"));
    return;
  }

  // The SPA has received the station address and can navigate to it. Keep the
  // portal alive briefly so the normal device web server can start cleanly.
  sendApiJson(request, 200, F("{\"ok\":true}"));
  _wm->acknowledgePortalConnectHandoff();
}

void WiFiManagerHandlers::handleApiPortalTimeoutReset(AsyncWebServerRequest *request) {
  handleRequest(request);
  if (!_wm->configPortalActive || _wm->_configPortalTimeout == 0) {
    sendApiJson(request, 409, F("{\"ok\":false,\"message\":\"Portal timeout is not active\"}"));
    return;
  }

  _wm->_configPortalStart = millis();
  String json = F("{\"ok\":true,\"timeoutSecondsRemaining\":");
  json += String((_wm->_configPortalTimeout + 999UL) / 1000UL);
  json += F("}");
  sendApiJson(request, 200, json);
}

String WiFiManagerHandlers::buildApiParamsGetJson() {
  String json = F("{\"params\":[");
  bool first = true;
  appendPortalJsonCustomParams(json, first);
  json += F("],\"actions\":{\"showBack\":");
  json += _wm->_portalActions.backVisible ? F("true") : F("false");
  json += F("}}");
  return json;
}

void WiFiManagerHandlers::handleApiParamsGet(AsyncWebServerRequest *request) {
  handleRequest(request);
  sendApiJson(request, 200, buildApiParamsGetJson());
}

void WiFiManagerHandlers::handleApiParamsSave(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("<- API params save"));
#endif
  handleRequest(request);
  WiFiManager::WiFiManagerRequestArgs requestArgs(request);
  doParamSave(requestArgs);
  sendApiJson(request, 200, jsonApiParamsSaveOk());
}

String WiFiManagerHandlers::buildApiInfoJson() {
  String json = F("{\"status\":{\"connected\":");
  json += WiFi.isConnected() ? F("true") : F("false");
  json += F(",\"ssid\":\"");
  jsonAppendEscaped(json, _wm->WiFi_SSID());
  json += F("\",\"stationIp\":\"");
  jsonAppendEscaped(json, WiFi.localIP().toString());
  json += F("\",\"apIp\":\"");
  jsonAppendEscaped(json, WiFi.softAPIP().toString());
  json += F("\",\"summary\":\"");
  {
    String summary;
    buildPlainStatusSummary(summary);
    jsonAppendEscaped(json, summary);
  }
  json += F("\"},\"device\":[");
  bool first = true;
#ifdef ESP8266
  static const char *const deviceIds[] = {"uptime",    "chipid",   "fchipid", "idesize", "flashsize",
                                          "corever",   "bootver",  "cpufreq", "freeheap", "memsketch",
                                          "memsmeter", "lastreset"};
#elif defined(ESP32)
  static const char *const deviceIds[] = {"uptime",   "chipid",   "chiprev", "idesize",  "flashsize",
                                          "cpufreq", "freeheap", "memsketch", "memsmeter", "lastreset", "temp"};
#endif
  appendInfoSectionFromIds(json, deviceIds, sizeof(deviceIds) / sizeof(deviceIds[0]), first);
  json += F("],\"wifi\":[");
  first = true;
#ifdef ESP8266
  static const char *const wifiIds[] = {"conx",   "stassid", "staip", "stagw", "stasub", "dnss", "host",
                                        "stamac", "autoconx", "apssid", "apip", "apbssid", "apmac"};
#elif defined(ESP32)
  static const char *const wifiIds[] = {"conx",   "stassid", "staip", "stagw", "stasub", "dnss", "host",
                                        "stamac", "apssid", "apip", "apmac", "aphost", "apbssid"};
#endif
  appendInfoSectionFromIds(json, wifiIds, sizeof(wifiIds) / sizeof(wifiIds[0]), first);
  json += F("],\"about\":[");
  first = true;
  static const char *const aboutIds[] = {"aboutver", "aboutarduinover", "aboutsdkver", "aboutdate"};
  appendInfoSectionFromIds(json, aboutIds, sizeof(aboutIds) / sizeof(aboutIds[0]), first);
  json += F("],\"extraSections\":[");
  first = true;
  appendPortalExtraInfoSectionsJson(json, first);
  json += F("],\"actions\":{");
  appendApiInfoActionsJson(json);
  json += F("}}");
  return json;
}

void WiFiManagerHandlers::handleApiInfo(AsyncWebServerRequest *request) {
  handleRequest(request);
  sendApiJson(request, 200, buildApiInfoJson());
}

String WiFiManagerHandlers::buildApiStatusJson() {
  String summary;
  buildPlainStatusSummary(summary);
  String json = F("{\"text\":\"");
  jsonAppendEscaped(json, summary);
  json += F("\"}");
  return json;
}

void WiFiManagerHandlers::handleApiStatus(AsyncWebServerRequest *request) {
  handleRequest(request);
  sendApiJson(request, 200, buildApiStatusJson());
}

String WiFiManagerHandlers::jsonApiWifiScanAccepted() {
  return F("{\"accepted\":true,\"state\":\"queued\"}");
}

String WiFiManagerHandlers::jsonApiDeviceRestartScheduled() {
  return F("{\"ok\":true,\"message\":\"Restart scheduled\"}");
}

String WiFiManagerHandlers::jsonApiParamsSaveOk() {
  return F("{\"ok\":true,\"message\":\"Setup saved\"}");
}

String WiFiManagerHandlers::jsonApiPortalCloseOk() {
  return F("{\"ok\":true,\"message\":\"Captive portal detection disabled\"}");
}

String WiFiManagerHandlers::jsonApiPortalExitOk() {
  return F("{\"ok\":true,\"message\":\"Exiting portal\"}");
}

String WiFiManagerHandlers::jsonApiPortalExitForbidden() {
  return F("{\"ok\":false,\"message\":\"Exit not allowed\"}");
}

String WiFiManagerHandlers::jsonApiOtaUpdateSuccess() {
  return F("{\"ok\":true,\"message\":\"Firmware updated. Restarting...\"}");
}

String WiFiManagerHandlers::jsonApiEraseResponse(boolean success) {
  if (success) {
    return F("{\"ok\":true,\"message\":\"WiFi configuration erased. Device will restart shortly.\"}");
  }
  return F("{\"ok\":false,\"message\":\"Erase failed\"}");
}

void WiFiManagerHandlers::handleApiDeviceRestart(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("API device restart"));
#endif
  handleRequest(request);
  sendApiJson(request, 200, jsonApiDeviceRestartScheduled());
  _wm->_rebootScheduled = true;
  _wm->_rebootTime = millis() + _wm->REBOOT_DELAY_MS;
}

void WiFiManagerHandlers::handleApiDeviceErase(AsyncWebServerRequest *request, boolean optionalErase) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("API device erase"));
#endif
  handleRequest(request);
  bool ret = _wm->erase(optionalErase);
  sendApiJson(request, ret ? 200 : 500, jsonApiEraseResponse(ret));
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
  sendApiJson(request, 200, jsonApiPortalCloseOk());
}

void WiFiManagerHandlers::handleApiPortalExit(AsyncWebServerRequest *request) {
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("API portal exit"));
#endif
  handleRequest(request);
  if (!_wm->_allowExit) {
    sendApiJson(request, 403, jsonApiPortalExitForbidden());
    return;
  }
  sendApiJson(request, 200, jsonApiPortalExitOk());
  _wm->_abortScheduled = true;
  _wm->_abortTime = millis() + _wm->EXIT_DELAY_MS;
}

#endif // defined(ESP8266) || defined(ESP32)

