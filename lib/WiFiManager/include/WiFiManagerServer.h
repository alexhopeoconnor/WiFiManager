/**
 * WiFiManagerServer.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * HTTP server for the config portal: single HTML shell (GET /), JSON under /api/..., OTA POST /u,
 * captive-portal redirects, and 404 handling. No legacy page routes.
 *
 * Supported surface (authoritative; registerRoutes mirrors this):
 *   GET  /
 *   GET  /api/bootstrap
 *   GET  /api/wifi/scan-status
 *   POST /api/wifi/scan
 *   GET  /api/wifi/meta
 *   POST /api/wifi/save
 *   GET  /api/wifi/connect-status
 *   GET  /api/params
 *   POST /api/params/save
 *   GET  /api/info
 *   GET  /api/status
 *   POST /api/device/restart
 *   POST /api/device/erase
 *   POST /api/portal/close
 *   POST /api/portal/exit
 *   POST /u                      (multipart firmware upload; JSON completion response)
 *   notFound -> captive redirect or 404
 */

#ifndef WiFiManagerServer_h
#define WiFiManagerServer_h

#if defined(ESP8266) || defined(ESP32)

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <functional>
#include <memory>

class WiFiManager;
class WiFiManagerHandlers;

#ifdef WM_DFTE_LOGGING
class WiFiManagerDfteLogger;
#endif

// -----------------------------------------------------------------------------------------------
// HTTP routes (portal API only; no legacy /wifi, /info, … page URLs)

const char R_root[]               PROGMEM = "/";

const char R_api_bootstrap[]      PROGMEM = "/api/bootstrap";
const char R_api_wifi_scan_status[] PROGMEM = "/api/wifi/scan-status";
const char R_api_wifi_scan[]      PROGMEM = "/api/wifi/scan";
const char R_api_wifi_meta[]      PROGMEM = "/api/wifi/meta";
const char R_api_wifi_save[]      PROGMEM = "/api/wifi/save";
const char R_api_wifi_connect_status[] PROGMEM = "/api/wifi/connect-status";
const char R_api_params[]         PROGMEM = "/api/params";
const char R_api_params_save[]    PROGMEM = "/api/params/save";
const char R_api_info[]           PROGMEM = "/api/info";
const char R_api_status[]         PROGMEM = "/api/status";
const char R_api_device_restart[] PROGMEM = "/api/device/restart";
const char R_api_device_erase[]   PROGMEM = "/api/device/erase";
const char R_api_portal_close[]   PROGMEM = "/api/portal/close";
const char R_api_portal_exit[]    PROGMEM = "/api/portal/exit";

const char R_updatedone[]         PROGMEM = "/u";

class WiFiManagerServer {
  public:
    WiFiManagerServer(WiFiManager* wm);
    ~WiFiManagerServer();

    void createServer(uint16_t port);
    void registerRoutes();
    void setupDNSD();
    void processDNS();
    void shutdownServer();
    AsyncWebServer* getServer() { return server.get(); }
    DNSServer* getDNSServer() { return dnsServer.get(); }

  private:
    WiFiManager* _wm;
    std::unique_ptr<WiFiManagerHandlers> _handlers;
    std::unique_ptr<AsyncWebServer> server;
    std::unique_ptr<DNSServer> dnsServer;

#ifdef WM_DFTE_LOGGING
    std::unique_ptr<WiFiManagerDfteLogger> _dfteLogger;
    bool _wmOwnsDfteLogSink = false;
#endif
};

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerServer_h
