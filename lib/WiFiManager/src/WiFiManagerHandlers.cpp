/**
 * WiFiManagerHandlers.cpp
 * 
 * HTTP request handlers and rendering logic implementation
 * 
 * @author alexhopeoconnor
 * @license MIT
 */

#include "WiFiManagerHandlers.h"
#include "WiFiManagerServer.h"
#include "templates/CSS.h"
#include "templates/JS.h"
#include "templates/RootShell.h"
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
    return;
  }

  registry.registerProgmemData("%STYLES%", CSS_STYLE);
  registry.registerProgmemData("%SCRIPTS%", JS_SCRIPT);
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

}  // namespace

WiFiManagerHandlers::WiFiManagerHandlers(WiFiManager* wm) : _wm(wm) {}

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
      json += F("{\"name\":\"");
      jsonAppendEscaped(json, pid);
      json += F("\",\"id\":\"");
      jsonAppendEscaped(json, pid);
      json += F("\",\"label\":\"");
      jsonAppendEscaped(json, String(p->getLabel()));
      json += F("\",\"type\":\"text\",\"maxlength\":");
      json += String(p->getValueLength());
      json += F(",\"value\":\"");
      jsonAppendEscaped(json, String(p->getValue()));
      json += F("\",\"labelPlacement\":");
      json += String(p->getLabelPlacement());
      {
        String c = p->getCustomHTML();
        if (c.length() > 0) {
          json += F(",\"customAttrs\":\"");
          jsonAppendEscaped(json, c);
          json += F("\"");
        }
      }
      json += F("}");
    } else {
      json += F("{\"html\":\"");
      jsonAppendEscaped(json, String(p->getCustomHTML()));
      json += F("}");
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
  json += F("}");
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

  auto bundle = std::make_shared<ShellRenderBundle>();
  bundle->bootstrapJson = buildPortalBootstrapJson();

  if (_wm->_serverManager) {
    _wm->_serverManager->registerDefaultStyles(bundle->registry);
    _wm->_serverManager->registerDefaultScripts(bundle->registry);
    _wm->_serverManager->registerDefaultPageTitle(bundle->registry);
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

void WiFiManagerHandlers::appendPortalUiFeatureFlagsJson(String& json) {
  const bool portalRunning = _wm->configPortalActive || _wm->webPortalActive;
  json += F("\"showInfo\":");
  json += _wm->_showInfo ? F("true") : F("false");
  json += F(",\"showUpdate\":");
  json += _wm->_showInfoUpdate ? F("true") : F("false");
  json += F(",\"showErase\":");
  json += _wm->_showInfoErase ? F("true") : F("false");
  json += F(",\"paramsInWifi\":");
  json += _wm->_paramsInWifi ? F("true") : F("false");
  json += F(",\"showRestart\":");
  json += portalRunning ? F("true") : F("false");
  json += F(",\"showExitPortal\":");
  json += (_wm->_allowExit && portalRunning) ? F("true") : F("false");
  json += F(",\"showCloseCaptive\":");
  json += (_wm->_enableCaptivePortal && _wm->configPortalActive) ? F("true") : F("false");
}

void WiFiManagerHandlers::appendApiInfoActionsJson(String& json) {
  const bool portalRunning = _wm->configPortalActive || _wm->webPortalActive;
  json += F("\"showUpdate\":");
  json += _wm->_showInfoUpdate ? F("true") : F("false");
  json += F(",\"showErase\":");
  json += _wm->_showInfoErase ? F("true") : F("false");
  json += F(",\"showBack\":");
  json += _wm->_showBack ? F("true") : F("false");
  json += F(",\"showRestart\":");
  json += portalRunning ? F("true") : F("false");
  json += F(",\"showExitPortal\":");
  json += (_wm->_allowExit && portalRunning) ? F("true") : F("false");
  json += F(",\"showCloseCaptive\":");
  json += (_wm->_enableCaptivePortal && _wm->configPortalActive) ? F("true") : F("false");
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
  appendPortalUiFeatureFlagsJson(json);
  json += F("},\"showBack\":");
  json += _wm->_showBack ? F("true") : F("false");
  json += F(",\"scan\":{\"state\":\"");
  json += stateStr;
  json += F("\",\"count\":");
  json += String((unsigned int)networks.size());
  json += F("},\"initialStatus\":\"");
  {
    String st;
    buildPlainStatusSummary(st);
    jsonAppendEscaped(json, st);
  }
  json += F("\"}");
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
      request->beginResponse(202, "application/json", "{\"accepted\":true,\"state\":\"queued\"}");
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

String WiFiManagerHandlers::buildApiWifiMetaJson() {
  String ssidPlaceholder = _wm->WiFi_SSID();
  String passwordPlaceholder = "";
  if (_wm->_showPassword) {
    passwordPlaceholder = _wm->WiFi_psk();
  } else if (_wm->WiFi_psk() != "") {
    passwordPlaceholder = F("********");
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
  if (_wm->_paramsInWifi && _wm->getParametersCount() > 0) {
    appendPortalJsonCustomParams(json, first);
  }
  json += F("],\"actions\":{\"canRefreshScan\":true,\"showBack\":");
  json += _wm->_showBack ? F("true") : F("false");
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

String WiFiManagerHandlers::buildApiParamsGetJson() {
  String json = F("{\"params\":[");
  bool first = true;
  appendPortalJsonCustomParams(json, first);
  json += F("],\"actions\":{\"showBack\":");
  json += _wm->_showBack ? F("true") : F("false");
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

