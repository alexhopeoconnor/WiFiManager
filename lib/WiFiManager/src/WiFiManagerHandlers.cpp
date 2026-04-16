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
#include "templates/PageShell.h"
#include "templates/Info.h"
#include "templates/WiFi.h"
#include "templates/RootSelector.h"
#include <TemplateEngine.h>
#include <cstring>

#if defined(ESP8266) || defined(ESP32)

#ifndef WM_PAGE_RESERVE_BYTES
#define WM_PAGE_RESERVE_BYTES 8192
#endif

#ifndef WM_ROOT_REQUEST_STATE
#define WM_ROOT_REQUEST_STATE 1
#endif

namespace {

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

struct RootState {
  String menu;
  String status;
};

struct MessagePageState {
  String docTitle;
  String bodyClass;
  String content;
};

struct InfoPageState {
  String docTitle;
  String status;
  String deviceSection;
  String wifiSection;
  String aboutSection;
};

struct WiFiPageState {
  String docTitle;
  String scanResults;
  String formSection;
  String backSection;
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

void buildRootState(WiFiManagerHandlers* handlers, RootState& state) {
  reservePage(state.menu, 768);
  reservePage(state.status, 512);

  handlers->getMenuOut(&state.menu);
  handlers->reportStatus(state.status);

  // Dynamic templates are parsed as template content, so escape literal '%' values.
  escapePercentsForTemplate(state.menu);
  escapePercentsForTemplate(state.status);
}

void appendPortalHeader(const String& title,
                        const String& subtitle,
                        String& content) {
  content += F("<h1>");
  content += title;
  content += F("</h1><h3>");
  content += subtitle;
  content += F("</h3>");
}

void buildMessagePageState(MessagePageState& state,
                           const __FlashStringHelper* title,
                           const __FlashStringHelper* bodyClass,
                           const String& contentHtml) {
  state.docTitle = String(title);
  state.bodyClass = String(bodyClass);
  state.content = contentHtml;
  escapePercentsForTemplate(state.content);
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

  state.docTitle = F("Info");
  handlers->reportStatus(state.status);

#ifdef ESP8266
  static const char* const deviceIds[] = {
    "esphead", "uptime", "chipid", "fchipid", "idesize", "flashsize",
    "corever", "bootver", "cpufreq", "freeheap", "memsketch", "memsmeter",
    "lastreset"
  };
  static const char* const wifiIds[] = {
    "wifihead", "conx", "stassid", "staip", "stagw", "stasub", "dnss",
    "host", "stamac", "autoconx", "wifiaphead", "apssid", "apip", "apbssid",
    "apmac"
  };
#elif defined(ESP32)
  static const char* const deviceIds[] = {
    "esphead", "uptime", "chipid", "chiprev", "idesize", "flashsize",
    "cpufreq", "freeheap", "memsketch", "memsmeter", "lastreset", "temp"
  };
  static const char* const wifiIds[] = {
    "wifihead", "conx", "stassid", "staip", "stagw", "stasub", "dnss",
    "host", "stamac", "apssid", "wifiaphead", "apip", "apmac", "aphost",
    "apbssid"
  };
#endif

  appendInfoEntries(handlers, state.deviceSection, deviceIds, sizeof(deviceIds) / sizeof(deviceIds[0]));
  state.deviceSection += F("</dl>");

  appendInfoEntries(handlers, state.wifiSection, wifiIds, sizeof(wifiIds) / sizeof(wifiIds[0]));
  state.wifiSection += F("</dl>");

  state.aboutSection += F("<h3>About</h3><hr><dl>");
  state.aboutSection += handlers->getInfoData("aboutver");
  state.aboutSection += handlers->getInfoData("aboutarduinover");
  state.aboutSection += handlers->getInfoData("aboutidfver");
  state.aboutSection += handlers->getInfoData("aboutdate");
  state.aboutSection += F("</dl>");

  if (showInfoUpdate) {
    state.aboutSection += HTML_PORTAL_MENU[8];
    state.aboutSection += HTML_PORTAL_MENU[9];
  }
  if (showInfoErase) state.aboutSection += FPSTR(HTML_ERASEBTN);
  if (showBack) state.aboutSection += FPSTR(HTML_BACKBTN);
  state.aboutSection += FPSTR(HTML_HELP);

  escapePercentsForTemplate(state.status);
  escapePercentsForTemplate(state.deviceSection);
  escapePercentsForTemplate(state.wifiSection);
  escapePercentsForTemplate(state.aboutSection);
}

void buildWiFiPageState(WiFiManagerHandlers* handlers,
                        WiFiPageState& state,
                        bool includeScanResults,
                        bool showBack,
                        const String& ssidPlaceholder,
                        const String& passwordPlaceholder,
                        bool paramsInWifi) {
  reservePage(state.scanResults, includeScanResults ? 4096 : 64);
  reservePage(state.formSection, 4096);
  reservePage(state.backSection, 256);
  reservePage(state.status, 512);

  state.docTitle = F("Config ESP");

  if (includeScanResults) {
    state.scanResults += F("<div id=\"scan-results\">");
    state.scanResults += handlers->getScanItemOut();
    state.scanResults += F("</div>");
  }

  state.formSection += F("<form method='POST' action='wifisave'>");

  state.formSection += F("<label for='s'>SSID</label><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='");
  state.formSection += ssidPlaceholder;
  state.formSection += F("'><br/><label for='p'>Password</label><input id='p' name='p' maxlength='64' type='password' placeholder='");
  state.formSection += passwordPlaceholder;
  state.formSection += F("'><input type='checkbox' id='showpass' onclick='f()'> <label for='showpass'>Show Password</label><br/>");

  state.formSection += handlers->getStaticOut();
  state.formSection += FPSTR(HTML_FORM_WIFI_END);
  if (paramsInWifi) {
    state.formSection += FPSTR(HTML_FORM_PARAM_HEAD);
    state.formSection += handlers->getParamOut();
  }
  state.formSection += FPSTR(HTML_FORM_END);
  state.formSection += F("<br/><div class=\"c\"><button id=\"refresh-btn\" onclick=\"refreshScan()\">Refresh</button></div>");

  if (showBack) {
    state.backSection += FPSTR(HTML_BACKBTN);
  }

  handlers->reportStatus(state.status);

  escapePercentsForTemplate(state.scanResults);
  escapePercentsForTemplate(state.formSection);
  escapePercentsForTemplate(state.backSection);
  escapePercentsForTemplate(state.status);
}

#if WM_ROOT_REQUEST_STATE
struct RootRenderBundle {
  RootState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor menuDescriptor;
  DynamicTemplateDescriptor statusDescriptor;

  RootRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY), menuDescriptor{}, statusDescriptor{} {}
};
#endif

struct MessageRenderBundle {
  MessagePageState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor docTitleDescriptor;
  DynamicTemplateDescriptor bodyClassDescriptor;
  DynamicTemplateDescriptor contentDescriptor;

  MessageRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        bodyClassDescriptor{},
        contentDescriptor{} {}
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

  InfoRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        statusDescriptor{},
        deviceDescriptor{},
        wifiDescriptor{},
        aboutDescriptor{} {}
};

struct WiFiRenderBundle {
  WiFiPageState state;
  PlaceholderRegistry registry;
  TemplateContext context;
  DynamicTemplateDescriptor docTitleDescriptor;
  DynamicTemplateDescriptor scanResultsDescriptor;
  DynamicTemplateDescriptor formDescriptor;
  DynamicTemplateDescriptor backDescriptor;
  DynamicTemplateDescriptor statusDescriptor;

  WiFiRenderBundle()
      : registry(WM_TEMPLATE_REGISTRY_CAPACITY),
        docTitleDescriptor{},
        scanResultsDescriptor{},
        formDescriptor{},
        backDescriptor{},
        statusDescriptor{} {}
};

}  // namespace

WiFiManagerHandlers::WiFiManagerHandlers(WiFiManager* wm) : _wm(wm) {}

// Rendering Methods

String WiFiManagerHandlers::getHTTPHead(String title, String classes){
  String page;
  reservePage(page, 512);
  page += FPSTR(HTML_HEAD_START);
  page += title;
  page += FPSTR(HTML_TITLE_END);
  page += FPSTR(JS_SCRIPT);
  page += FPSTR(CSS_STYLE);
  page += FPSTR(HTML_HEAD_END_START);
  page += classes;
  page += FPSTR(HTML_HEAD_END_WRAP);
  return page;
}

String WiFiManagerHandlers::getHTTPEnd() {
  return FPSTR(HTML_END);
}

String WiFiManagerHandlers::getMenuOut(){
  return getMenuOut(nullptr);
}

String WiFiManagerHandlers::getMenuOut(String* outOpt){
  String local;
  String &out = outOpt ? *outOpt : local;
  out += HTML_PORTAL_MENU[0]; // WIFI (scan)
  
  // Show PARAM (Setup) when params are on their own page
  if(!_wm->_paramsInWifi && _wm->_paramsCount > 0){
    out += HTML_PORTAL_MENU[3]; // PARAM
  }
  
  out += HTML_PORTAL_MENU[2]; // INFO
  
  // When captive/config portal is active, offer Close (keeps portal running)
  if(_wm->configPortalActive){
    out += HTML_PORTAL_MENU[4]; // CLOSE
  }
  
  out += HTML_PORTAL_MENU[6]; // EXIT
  out += HTML_PORTAL_MENU[9]; // SEP
  out += HTML_PORTAL_MENU[8]; // UPDATE
  return out;
}

String WiFiManagerHandlers::getScanItemOut(){
    String page;
    int n = _wm->_numNetworks;
    reservePage(page, 256 + (n > 0 ? static_cast<size_t>(n) * 96 : 96));

    // Never trigger scans from here - only use cached data
    // If no cached data, show message (scan should be started elsewhere if needed)
    if (n == 0) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(F("No networks found"));
      #endif
      if(_wm->_scanInProgress){
        page += F("Scanning for networks...<br/><br/>");
      } else {
        page += F("No networks found. Refresh to scan again.");
        page += F("<br/><br/>");
      }
    }
    else {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(n,F("networks found"));
      #endif
      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }

      // RSSI SORT
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      // remove duplicates ( must be RSSI sorted )
      if (_wm->_removeDuplicateAPs) {
        String cssid;
        for (int i = 0; i < n; i++) {
          if (indices[i] == -1) continue;
          cssid = WiFi.SSID(indices[i]);
          for (int j = i + 1; j < n; j++) {
            if (cssid == WiFi.SSID(indices[j])) {
              #ifdef WM_DEBUG_LEVEL
              _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("DUP AP:"),WiFi.SSID(indices[j]));
              #endif
              indices[j] = -1; // set dup aps to index -1
            }
          }
        }
      }

      // Build network items directly without tokens
      String hiddenClass = _wm->_scanDispOptions ? "" : "h";
      String visibleClass = _wm->_scanDispOptions ? "h" : "";
      
      //display networks in page
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups

        #ifdef WM_DEBUG_LEVEL
        _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("AP: "),(String)WiFi.RSSI(indices[i]) + " " + (String)WiFi.SSID(indices[i]));
        #endif

        int rssiperc = _wm->getRSSIasQuality(WiFi.RSSI(indices[i]));
        uint8_t enc_type = WiFi.encryptionType(indices[i]);

        if (_wm->_minimumQuality == -1 || _wm->_minimumQuality < rssiperc) {
          if(WiFi.SSID(indices[i]) == ""){
            continue; // No idea why I am seeing these, lets just skip them for now
          }
          
          String ssid_encoded = _wm->htmlEntities(WiFi.SSID(indices[i]));
          String ssid_display = _wm->htmlEntities(WiFi.SSID(indices[i]), true);
          int quality = int(round(map(rssiperc,0,100,1,4)));
          String lockIcon = (enc_type != WM_WIFIOPEN) ? "l" : "";
          
          // Build item HTML directly
          page += F("<div><a href='#p' onclick='c(this)' data-ssid='");
          page += ssid_encoded;
          page += F("'>");
          page += ssid_display;
          page += F("</a>");
          
          // Add RSSI quality icon
          page += F("<div role='img' aria-label='");
          page += String(rssiperc);
          page += F("%' title='");
          page += String(rssiperc);
          page += F("%' class='q q-");
          page += String(quality);
          page += F(" ");
          page += lockIcon;
          page += F(" ");
          page += hiddenClass;
          page += F("'></div>");
          
          // Add RSSI percentage (if showing percentage)
          page += F("<div class='q ");
          page += visibleClass;
          page += F("'>");
          page += String(rssiperc);
          page += F("%</div></div>");
          
          #ifdef WM_DEBUG_LEVEL
          _wm->DEBUG_WM(WM_DEBUG_DEV, F("Added network item"));
          #endif
          delay(0);
        } else {
          #ifdef WM_DEBUG_LEVEL
          _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("Skipping , does not meet _minimumQuality"));
          #endif
        }

      }
      page += FPSTR(HTML_BR);
    }

    return page;
}

String WiFiManagerHandlers::getIpForm(String id, String title, String value){
    // Build IP form field directly without tokens
    String item = F("<label for='");
    item += id;
    item += F("'>");
    item += title;
    item += F("</label><br/><input id='");
    item += id;
    item += F("' name='");
    item += id;
    item += F("' maxlength='15' value='");
    item += value;
    item += F("'>");
    return item;  
}

String WiFiManagerHandlers::getStaticOut(){
  String page;
  reservePage(page, 384);
  if ((_wm->_staShowStaticFields || _wm->_sta_static_ip) && _wm->_staShowStaticFields>=0) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV,F("_staShowStaticFields"));
    #endif
    page += FPSTR(HTML_FORM_STATIC_HEAD);
    page += getIpForm(FPSTR(S_ip),F("Static IP"),(_wm->_sta_static_ip ? _wm->_sta_static_ip.toString() : ""));
    page += getIpForm(FPSTR(S_gw),F("Static gateway"),(_wm->_sta_static_gw ? _wm->_sta_static_gw.toString() : ""));
    page += getIpForm(FPSTR(S_sn),F("Subnet"),(_wm->_sta_static_sn ? _wm->_sta_static_sn.toString() : ""));
  }

  if((_wm->_staShowDns || _wm->_sta_static_dns) && _wm->_staShowDns>=0){
    page += getIpForm(FPSTR(S_dns),F("Static DNS"),(_wm->_sta_static_dns ? _wm->_sta_static_dns.toString() : ""));
  }

  if(page!="") page += FPSTR(HTML_BR);

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
        String paramLabel = _wm->_params[i]->getLabel();
        String paramValue = _wm->_params[i]->getValue();
        String customHTML = _wm->_params[i]->getCustomHTML();
        snprintf(valLength, 5, "%d", _wm->_params[i]->getValueLength());
        
        // Build parameter HTML directly based on label placement
        switch (_wm->_params[i]->getLabelPlacement()) {
          case WFM_LABEL_BEFORE:
            // Label before input
            pitem = F("<label for='");
            pitem += paramId;
            pitem += F("'>");
            pitem += paramLabel;
            pitem += F("</label><br/><input id='");
            pitem += paramId;
            pitem += F("' name='");
            pitem += paramId;
            pitem += F("' maxlength='");
            pitem += String(valLength);
            pitem += F("' value='");
            pitem += paramValue;
            pitem += F("'");
            if(customHTML.length() > 0) {
              pitem += F(" ");
              pitem += customHTML;
            }
            pitem += F(">\n");
            break;
          case WFM_LABEL_AFTER:
            // Label after input
            pitem = F("<br/><input id='");
            pitem += paramId;
            pitem += F("' name='");
            pitem += paramId;
            pitem += F("' maxlength='");
            pitem += String(valLength);
            pitem += F("' value='");
            pitem += paramValue;
            pitem += F("'");
            if(customHTML.length() > 0) {
              pitem += F(" ");
              pitem += customHTML;
            }
            pitem += F(">\n<label for='");
            pitem += paramId;
            pitem += F("'>");
            pitem += paramLabel;
            pitem += F("</label>");
            break;
          default:
            // WFM_NO_LABEL - no label
            pitem = F("<br/><input id='");
            pitem += paramId;
            pitem += F("' name='");
            pitem += paramId;
            pitem += F("' maxlength='");
            pitem += String(valLength);
            pitem += F("' value='");
            pitem += paramValue;
            pitem += F("'");
            if(customHTML.length() > 0) {
              pitem += F(" ");
              pitem += customHTML;
            }
            pitem += F(">\n");
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
  if(id==F("esphead")){
    #ifdef ESP32
      p = F("<h3>esp32</h3><hr><dl>");
    #else
      p = F("<h3>esp8266</h3><hr><dl>");
    #endif
  }
  else if(id==F("wifihead")){
    p = F("<br/><h3>WiFi</h3><hr>");
  }
  else if(id==F("uptime")){
    p = F("<dt>Uptime</dt><dd>");
    p += String(millis() / 1000 / 60);
    p += F(" mins ");
    p += String((millis() / 1000) % 60);
    p += F(" secs</dd>");
  }
  else if(id==F("chipid")){
    p = F("<dt>Chip ID</dt><dd>");
    #ifdef ESP8266
      p += String(ESP.getChipId(),HEX);
    #elif defined(ESP32)
      p += String((uint32_t)ESP.getEfuseMac(),HEX);
    #endif
    p += F("</dd>");
  }
  #ifdef ESP32
  else if(id==F("chiprev")){
      p = F("<dt>Chip rev</dt><dd>");
      String rev = (String)ESP.getChipRevision();
      #ifdef _SOC_EFUSE_REG_H_
        String revb = (String)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S)&&EFUSE_RD_CHIP_VER_RESERVE_V);
        p += rev;
        p += F("<br/>");
        p += revb;
      #else
        p += rev;
      #endif
      p += F("</dd>");
  }
  #endif
  #ifdef ESP8266
  else if(id==F("fchipid")){
      p = F("<dt>Flash chip ID</dt><dd>");
      p += String(ESP.getFlashChipId());
      p += F("</dd>");
  }
  #endif
  else if(id==F("idesize")){
    p = F("<dt>Flash size</dt><dd>");
    p += String(ESP.getFlashChipSize());
    p += F(" bytes</dd>");
  }
  else if(id==F("flashsize")){
    #ifdef ESP8266
      p = F("<dt>Real flash size</dt><dd>");
      p += String(ESP.getFlashChipRealSize());
      p += F(" bytes</dd>");
    #elif defined ESP32
      p = F("<dt>PSRAM Size</dt><dd>");
      p += String(ESP.getPsramSize());
      p += F(" bytes</dd>");
    #endif
  }
  else if(id==F("corever")){
    #ifdef ESP8266
      p = F("<dt>Core version</dt><dd>");
      p += String(ESP.getCoreVersion());
      p += F("</dd>");
    #endif      
  }
  #ifdef ESP8266
  else if(id==F("bootver")){
      p = F("<dt>Boot version</dt><dd>");
      p += String(system_get_boot_version());
      p += F("</dd>");
  }
  #endif
  else if(id==F("cpufreq")){
    p = F("<dt>CPU frequency</dt><dd>");
    p += String(ESP.getCpuFreqMHz());
    p += F("MHz</dd>");
  }
  else if(id==F("freeheap")){
    p = F("<dt>Memory - Free heap</dt><dd>");
    p += String(ESP.getFreeHeap());
    p += F(" bytes available</dd>");
  }
  else if(id==F("memsketch")){
    p = F("<dt>Memory - Sketch size</dt><dd>Used / Total bytes<br/>");
    p += String(ESP.getSketchSize());
    p += F(" / ");
    p += String(ESP.getSketchSize()+ESP.getFreeSketchSpace());
    p += F("</dd>");
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
      p = F("<dt>Last reset reason</dt><dd>");
      p += String(ESP.getResetReason());
      p += F("</dd>");
    #elif defined(ESP32) && defined(_ROM_RTC_H_)
      p = F("<dt>Last reset reason</dt><dd>CPU0: ");
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
      p += reasons[0];
      p += F("<br/>CPU1: ");
      p += reasons[1];
      p += F("</dd>");
    #endif
  }
  else if(id==F("apip")){
    p = F("<dt>Access point IP</dt><dd>");
    p += WiFi.softAPIP().toString();
    p += F("</dd>");
  }
  else if(id==F("apmac")){
    p = F("<dt>Access point MAC</dt><dd>");
    p += WiFi.softAPmacAddress();
    p += F("</dd>");
  }
  #ifdef ESP32
  else if(id==F("aphost")){
      p = F("<dt>Access point hostname</dt><dd>");
      p += WiFi.softAPgetHostname();
      p += F("</dd>");
  }
  #endif
  #ifndef WM_NOSOFTAPSSID
  #ifdef ESP8266
  else if(id==F("apssid")){
    p = F("<dt>Access point SSID</dt><dd>");
    p += _wm->htmlEntities(WiFi.softAPSSID());
    p += F("</dd>");
  }
  #endif
  #endif
  else if(id==F("apbssid")){
    p = F("<dt>BSSID</dt><dd>");
    p += WiFi.BSSIDstr();
    p += F("</dd>");
  }
  else if(id==F("stassid")){
    p = F("<dt>Station SSID</dt><dd>");
    p += _wm->htmlEntities((String)_wm->WiFi_SSID());
    p += F("</dd>");
  }
  else if(id==F("staip")){
    p = F("<dt>Station IP</dt><dd>");
    p += WiFi.localIP().toString();
    p += F("</dd>");
  }
  else if(id==F("stagw")){
    p = F("<dt>Station gateway</dt><dd>");
    p += WiFi.gatewayIP().toString();
    p += F("</dd>");
  }
  else if(id==F("stasub")){
    p = F("<dt>Station subnet</dt><dd>");
    p += WiFi.subnetMask().toString();
    p += F("</dd>");
  }
  else if(id==F("dnss")){
    p = F("<dt>DNS Server</dt><dd>");
    p += WiFi.dnsIP().toString();
    p += F("</dd>");
  }
  else if(id==F("host")){
    p = F("<dt>Hostname</dt><dd>");
    #ifdef ESP32
      p += WiFi.getHostname();
    #else
      p += WiFi.hostname();
    #endif
    p += F("</dd>");
  }
  else if(id==F("stamac")){
    p = F("<dt>Station MAC</dt><dd>");
    p += WiFi.macAddress();
    p += F("</dd>");
  }
  else if(id==F("conx")){
    p = F("<dt>Connected</dt><dd>");
    p += (WiFi.isConnected() ? F("Yes") : F("No"));
    p += F("</dd>");
  }
  #ifdef ESP8266
  else if(id==F("autoconx")){
    p = F("<dt>Autoconnect</dt><dd>");
    p += (WiFi.getAutoConnect() ? F("Enabled") : F("Disabled"));
    p += F("</dd>");
  }
  #endif
  #if defined(ESP32) && !defined(WM_NOTEMP)
  else if(id==F("temp")){
    p = F("<dt>Temperature</dt><dd>");
    p += String(temperatureRead());
    p += F(" C&deg; / ");
    p += String((temperatureRead()+32)*1.8f);
    p += F(" F&deg;</dd>");
  }
  #endif
  else if(id==F("aboutver")){
    p = F("<dt>WiFiManager</dt><dd>");
    p += FPSTR(WM_VERSION_STR);
    p += F("</dd>");
  }
  else if(id==F("aboutarduinover")){
    #ifdef VER_ARDUINO_STR
    p = F("<dt>Arduino</dt><dd>");
    p += String(VER_ARDUINO_STR);
    p += F("</dd>");
    #endif
  }
  else if(id==F("aboutsdkver")){
    p = F("<dt>SDK version</dt><dd>");
    #ifdef ESP32
      p += String(esp_get_idf_version());
    #else
      p += String(system_get_sdk_version());
    #endif
    p += F("</dd>");
  }
  else if(id==F("aboutdate")){
    p = F("<dt>Build date</dt><dd>");
    p += String(__DATE__ " " __TIME__);
    p += F("</dd>");
  }
  return p;
}

void WiFiManagerHandlers::reportStatus(String &page){
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("[WIFI] reportStatus prev:"),_wm->getWLStatusString(_wm->_lastconxresult));
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("[WIFI] reportStatus current:"),_wm->getWLStatusString(WiFi.status()));
  String str;
  if (_wm->WiFi_SSID() != ""){
    if (WiFi.status()==WL_CONNECTED){
      // Build connected status HTML directly
      str = F("<div class='msg S'><strong>Connected</strong> to ");
      str += _wm->htmlEntities(_wm->WiFi_SSID());
      str += F("<br/><em><small>with IP ");
      str += WiFi.localIP().toString();
      str += F("</small></em></div>");
    }
    else {
      // Build disconnected status HTML directly
      String ssidEncoded = _wm->htmlEntities(_wm->WiFi_SSID());
      String statusClass = "D";
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
      
      str = F("<div class='msg ");
      str += statusClass;
      str += F("'><strong>Not connected</strong> to ");
      str += ssidEncoded;
      str += statusMsg;
      str += F("</div>");
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
#if WM_ROOT_REQUEST_STATE
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
#else
  TemplateContext ctx;
  if (_wm->_serverManager && _wm->_serverManager->getPlaceholderRegistry()) {
    ctx.setRegistry(_wm->_serverManager->getPlaceholderRegistry());
  }
  TemplateRenderer::initializeContext(ctx, WM_ROOT_TEMPLATE);
  auto ctxPtr = std::make_shared<TemplateContext>(ctx);

  response = request->beginChunkedResponse(String(FPSTR(HTTP_HEAD_CT)),
    [ctxPtr](uint8_t *buffer, size_t maxLen, size_t /*index*/) -> size_t {
      if (!ctxPtr) return 0;
      return TemplateRenderer::renderNextChunk(*ctxPtr, buffer, maxLen);
    }
  );
#endif

  request->send(response);
  if(_wm->_preloadwifiscan) _wm->WiFi_scanNetworks(_wm->_scancachetime);
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
  if (scan) {
    bool forceRefresh = false;
    if (request->hasParam("refresh")) {
      forceRefresh = true;
    }

    if(forceRefresh || !_wm->_lastscan || (millis()-_wm->_lastscan > _wm->_scancachetime)){
      if(!_wm->_scanInProgress){
        _wm->WiFi_scanNetworks(true);
      } else {
        _wm->_scanRequested = true;
      }
    }
  }

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
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->scanResultsDescriptor, bundle->state.scanResults);
  configureDynamicStringDescriptor(bundle->formDescriptor, bundle->state.formSection);
  configureDynamicStringDescriptor(bundle->backDescriptor, bundle->state.backSection);
  configureDynamicStringDescriptor(bundle->statusDescriptor, bundle->state.status);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_SCAN_RESULTS%", &bundle->scanResultsDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_FORM_SECTION%", &bundle->formDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_BACK_SECTION%", &bundle->backDescriptor);
  bundle->registry.registerDynamicTemplate("%WIFI_STATUS%", &bundle->statusDescriptor);

  #ifdef WM_DEBUG_LEVEL
  size_t debugPageLength = bundle->state.scanResults.length() + bundle->state.formSection.length()
                         + bundle->state.backSection.length() + bundle->state.status.length();
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Page length: "), String(debugPageLength));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_numNetworks: "), String(_wm->_numNetworks));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_scanInProgress: "), _wm->_scanInProgress ? "true" : "false");
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_lastscan: "), String(_wm->_lastscan));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("About to send response"));
  #endif

  request->send(beginTemplateResponse(request, bundle, WM_WIFI_TEMPLATE));

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Response sent"));
  #endif
}

void WiFiManagerHandlers::handleParam(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Param"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(F("Setup"), FPSTR(C_param));
  reservePage(page, 4096);

  // Build form start directly without tokens
  page += F("<form method='POST' action='paramsave'>");

  page += getParamOut();
  page += FPSTR(HTML_FORM_END);
  if(_wm->_showBack) page += FPSTR(HTML_BACKBTN);
  reportStatus(page);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

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

  String page;

  if(_wm->_ssid == ""){
    page = getHTTPHead(F("Settings saved"), FPSTR(C_wifi));
    reservePage(page, 1024);
    page += FPSTR(HTML_PARAMSAVED);
  }
  else {
    page = getHTTPHead(F("Credentials saved"), FPSTR(C_wifi));
    reservePage(page, 1024);
    page += FPSTR(HTML_SAVED);
  }

  if(_wm->_showBack) page += FPSTR(HTML_BACKBTN);
  page += getHTTPEnd();

  AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(HTTP_HEAD_CT), page);
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

  String page = getHTTPHead(F("Setup saved"), FPSTR(C_param));
  reservePage(page, 1024);
  page += FPSTR(HTML_PARAMSAVED);
  if(_wm->_showBack) page += FPSTR(HTML_BACKBTN); 
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

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
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->statusDescriptor, bundle->state.status);
  configureDynamicStringDescriptor(bundle->deviceDescriptor, bundle->state.deviceSection);
  configureDynamicStringDescriptor(bundle->wifiDescriptor, bundle->state.wifiSection);
  configureDynamicStringDescriptor(bundle->aboutDescriptor, bundle->state.aboutSection);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_STATUS%", &bundle->statusDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_DEVICE_SECTION%", &bundle->deviceDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_WIFI_SECTION%", &bundle->wifiDescriptor);
  bundle->registry.registerDynamicTemplate("%INFO_ABOUT_SECTION%", &bundle->aboutDescriptor);

  request->send(beginTemplateResponse(request, bundle, WM_INFO_TEMPLATE));

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
  buildMessagePageState(bundle->state, F("Exit"), FPSTR(C_exit), String(F("Exiting")));

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->bodyClassDescriptor, bundle->state.bodyClass);
  configureDynamicStringDescriptor(bundle->contentDescriptor, bundle->state.content);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%BODY_CLASS%", &bundle->bodyClassDescriptor);
  bundle->registry.registerDynamicTemplate("%PAGE_CONTENT%", &bundle->contentDescriptor);

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
  buildMessagePageState(bundle->state,
                        F("Reset"),
                        FPSTR(C_restart),
                        String(F("Module will reset in a few seconds.")));

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->bodyClassDescriptor, bundle->state.bodyClass);
  configureDynamicStringDescriptor(bundle->contentDescriptor, bundle->state.content);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%BODY_CLASS%", &bundle->bodyClassDescriptor);
  bundle->registry.registerDynamicTemplate("%PAGE_CONTENT%", &bundle->contentDescriptor);

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
  String content;
  reservePage(content, 256);

  if(ret) content += F("Module will reset in a few seconds.");
  else {
    content += F("An error occured");
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] WiFi EraseConfig failed"));
    #endif
  }

  buildMessagePageState(bundle->state, F("Erase"), FPSTR(C_erase), content);

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->bodyClassDescriptor, bundle->state.bodyClass);
  configureDynamicStringDescriptor(bundle->contentDescriptor, bundle->state.content);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%BODY_CLASS%", &bundle->bodyClassDescriptor);
  bundle->registry.registerDynamicTemplate("%PAGE_CONTENT%", &bundle->contentDescriptor);

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
  buildMessagePageState(bundle->state,
                        F("Close"),
                        FPSTR(C_close),
                        String(F("You can close the page, portal will continue to run")));

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->bodyClassDescriptor, bundle->state.bodyClass);
  configureDynamicStringDescriptor(bundle->contentDescriptor, bundle->state.content);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%BODY_CLASS%", &bundle->bodyClassDescriptor);
  bundle->registry.registerDynamicTemplate("%PAGE_CONTENT%", &bundle->contentDescriptor);

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

void WiFiManagerHandlers::handleWiFiStatus(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi status "));
  #endif
  handleRequest(request);
  String page;
  reservePage(page, 256);
  #ifdef WM_JSTEST
    page = FPSTR(HTML_JS);
  #endif
  request->send(200, FPSTR(HTTP_HEAD_CT), page);
}

void WiFiManagerHandlers::handleWiFiScanStatus(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi scan status"));
  #endif
  handleRequest(request);
  
  String json = "{";
  reservePage(json, 128 + (_wm->_numNetworks > 0 ? static_cast<size_t>(_wm->_numNetworks) * 96 : 64));
  json += "\"scanning\":";
  json += _wm->_scanInProgress ? "true" : "false";
  json += ",\"count\":";
  json += String(_wm->_numNetworks);
  json += ",\"lastscan\":";
  json += String(_wm->_lastscan);
  
  if(!_wm->_scanInProgress && _wm->_numNetworks > 0){
    json += ",\"networks\":[";
    
    int n = _wm->_numNetworks;
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    
    if (_wm->_removeDuplicateAPs) {
      String cssid;
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue;
        cssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++) {
          if (cssid == WiFi.SSID(indices[j])) {
            indices[j] = -1;
          }
        }
      }
    }
    
    bool first = true;
    for (int i = 0; i < n; i++) {
      if (indices[i] == -1) continue;
      
      if (!first) json += ",";
      first = false;
      
      json += "{";
      json += "\"ssid\":\"" + _wm->htmlEntities(WiFi.SSID(indices[i]), true) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(indices[i])) + ",";
      json += "\"encryption\":" + String(WiFi.encryptionType(indices[i]));
      json += "}";
    }
    json += "]";
  }
  
  json += "}";
  
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

void WiFiManagerHandlers::handleUpdate(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Handle update"));
  #endif
  if (captivePortal(request)) return;
  String page = getHTTPHead(_wm->_title, FPSTR(C_update));
  reservePage(page, 2048);
  // Build root main HTML directly without tokens
  page += F("<h1>");
  page += _wm->_title;
  page += F("</h1><h3>");
  page += (_wm->configPortalActive ? _wm->_apName : (_wm->getWiFiHostname() + " - " + WiFi.localIP().toString()));
  page += F("</h3>");

  page += FPSTR(HTML_UPDATE);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);
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

  auto bundle = std::make_shared<MessageRenderBundle>();
  String content;
  reservePage(content, 2048);
  appendPortalHeader(_wm->_title,
                     (_wm->configPortalActive ? _wm->_apName : WiFi.localIP().toString()),
                     content);

  if (Update.hasError()) {
    content += FPSTR(HTML_UPDATE_FAIL);
    #ifdef ESP32
    content += "OTA Error: " + (String)Update.errorString();
    #else
    content += "OTA Error: " + (String)Update.getError();
    #endif
  } else {
    content += FPSTR(HTML_UPDATE_SUCCESS);
  }

  buildMessagePageState(bundle->state, F("options"), FPSTR(C_update), content);

  registerSharedShellPlaceholders(_wm->_serverManager.get(), bundle->registry);
  configureDynamicStringDescriptor(bundle->docTitleDescriptor, bundle->state.docTitle);
  configureDynamicStringDescriptor(bundle->bodyClassDescriptor, bundle->state.bodyClass);
  configureDynamicStringDescriptor(bundle->contentDescriptor, bundle->state.content);

  bundle->registry.registerDynamicTemplate("%DOC_TITLE%", &bundle->docTitleDescriptor);
  bundle->registry.registerDynamicTemplate("%BODY_CLASS%", &bundle->bodyClassDescriptor);
  bundle->registry.registerDynamicTemplate("%PAGE_CONTENT%", &bundle->contentDescriptor);

  request->send(beginTemplateResponse(request, bundle, WM_PAGE_SHELL_TEMPLATE));

  if (!Update.hasError()) {
    delay(1000);
    ESP.restart();
  }
}

#endif // defined(ESP8266) || defined(ESP32)

