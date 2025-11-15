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
    void handleWiFiStatus(AsyncWebServerRequest *request);
    void handleWiFiScanStatus(AsyncWebServerRequest *request);
    void handleUpdate(AsyncWebServerRequest *request);
    void handleUpdating(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdateDone(AsyncWebServerRequest *request);
    
    // Captive Portal
    boolean captivePortal(AsyncWebServerRequest *request);
    void stopCaptivePortal();
    
    // Rendering Methods
    String getHTTPHead(String title, String classes = "");
    String getHTTPEnd();
    String getMenuOut();
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
};

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerHandlers_h

