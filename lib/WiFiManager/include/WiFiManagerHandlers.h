/**
 * WiFiManagerHandlers.h
 * 
 * HTTP request handlers and rendering logic for WiFiManager
 * Handles all web portal page generation and request processing
 * 
 * @author tablatronix
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#ifndef WiFiManagerHandlers_h
#define WiFiManagerHandlers_h

#if defined(ESP8266) || defined(ESP32)

#include <ESPAsyncWebServer.h>
#include <memory>
#include "WiFiManager.h" // Need full definition for WiFiManagerRequestArgs

// -----------------------------------------------------------------------------------------------
// CSS CLASSES (for body class attribute)

const char C_root[]               PROGMEM = "home";
const char C_wifi[]               PROGMEM = "wifi";
const char C_info[]               PROGMEM = "info";
const char C_param[]              PROGMEM = "param";
const char C_close[]              PROGMEM = "close";
const char C_restart[]            PROGMEM = "restart";
const char C_exit[]               PROGMEM = "exit";
const char C_erase[]              PROGMEM = "erase";
const char C_update[]             PROGMEM = "update";

// -----------------------------------------------------------------------------------------------
// FORM FIELD NAMES (for IP configuration forms)

const char S_ip[]                 PROGMEM = "ip";
const char S_gw[]                 PROGMEM = "gw";
const char S_sn[]                 PROGMEM = "sn";
const char S_dns[]                PROGMEM = "dns";

// -----------------------------------------------------------------------------------------------
// HTTP HEADERS

const char HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char HTTP_HEAD_CT2[]        PROGMEM = "text/plain";
const char HTTP_HEAD_CORS[]       PROGMEM = "Access-Control-Allow-Origin";
const char HTTP_HEAD_CORS_ALLOW_ALL[]  PROGMEM = "*";

class WiFiManagerHandlers {
  public:
    WiFiManagerHandlers(WiFiManager* wm);
    
    // HTTP Request Handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleWifi(AsyncWebServerRequest *request, boolean scan);
    void handleWifiSave(AsyncWebServerRequest *request);
    void handleParam(AsyncWebServerRequest *request);
    void handleParamSave(AsyncWebServerRequest *request);
    void handleInfo(AsyncWebServerRequest *request);
    void handleReset(AsyncWebServerRequest *request);
    void handleExit(AsyncWebServerRequest *request);
    void handleClose(AsyncWebServerRequest *request);
    void handleErase(AsyncWebServerRequest *request, boolean opt);
    void handleNotFound(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
    void handleWiFiScanRequest(AsyncWebServerRequest *request);
    void handleWiFiScanStatus(AsyncWebServerRequest *request);
    void handleUpdate(AsyncWebServerRequest *request);
    void handleUpdating(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdateDone(AsyncWebServerRequest *request);
    
    // Captive Portal
    boolean captivePortal(AsyncWebServerRequest *request);
    void stopCaptivePortal();
    
    // Rendering Methods
    String getMenuOut();
    String getMenuOut(String* outOpt);
    String getScanItemOut();
    String getParamOut();
    String getStaticOut();
    String getIpForm(String id, String title, String value);
    String getInfoData(String id);
    void reportStatus(String &page);
    
    // Internal helper (needed by handlers)
    void doParamSave(WiFiManager::WiFiManagerRequestArgs requestArgs);
    
  private:
    WiFiManager* _wm;
    void collectVisibleScanResults(std::vector<const WiFiManager::WiFiScanNetwork*>& networks);
    void appendVisibleScanResultsJson(String& json, const std::vector<const WiFiManager::WiFiScanNetwork*>& networks);
};

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerHandlers_h

