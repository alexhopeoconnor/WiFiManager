/**
 * WiFiManagerServer.h
 * 
 * Server lifecycle management for WiFiManager
 * Handles AsyncWebServer creation, route registration, and shutdown
 * 
 * @author tablatronix
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#ifndef WiFiManagerServer_h
#define WiFiManagerServer_h

#if defined(ESP8266) || defined(ESP32)

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <functional>
#include <memory>

// DFTE (template engine)
#include <TemplateEngine.h>

#ifndef WM_TEMPLATE_REGISTRY_CAPACITY
#define WM_TEMPLATE_REGISTRY_CAPACITY 24
#endif

// Forward declarations
class WiFiManager;

// Forward declaration - full definition needed in .cpp file
class WiFiManagerHandlers;

// -----------------------------------------------------------------------------------------------
// HTTP ROUTES

const char R_root[]               PROGMEM = "/";
const char R_wifi[]               PROGMEM = "/wifi";
const char R_wifinoscan[]         PROGMEM = "/0wifi";
const char R_wifisave[]           PROGMEM = "/wifisave";
const char R_info[]               PROGMEM = "/info";
const char R_param[]              PROGMEM = "/param";
const char R_paramsave[]          PROGMEM = "/paramsave";
const char R_restart[]            PROGMEM = "/restart";
const char R_exit[]               PROGMEM = "/exit";
const char R_close[]              PROGMEM = "/close";
const char R_erase[]              PROGMEM = "/erase";
const char R_scanstatus[]         PROGMEM = "/wifistatus";
const char R_scan[]               PROGMEM = "/wifi/scan";
const char R_update[]             PROGMEM = "/update";
const char R_updatedone[]         PROGMEM = "/u";

class WiFiManagerServer {
  public:
    // Singleton access
    static WiFiManagerServer* instance();
    
    WiFiManagerServer(WiFiManager* wm);
    
    // Template engine setup and access
    void setupTemplateEngine();
    PlaceholderRegistry* getPlaceholderRegistry() { return _tplRegistry.get(); }
    
    // Static zero-arg getters for DFTE RAM placeholders
    static const char* tplGetPageTitle();
    static const char* tplGetSubtitle();
    static const char* tplGetMenu();
    static const char* tplGetStatus();
    
    // Template customization API
    // Consumers can register a callback to customize placeholders
    void registerTemplateSetupCallback(std::function<void(PlaceholderRegistry&)> cb) { _tplSetupCallback = cb; }
    
    // Convenience helpers to register our defaults (granular + all)
    void registerDefaultStyles(PlaceholderRegistry& reg);
    void registerDefaultScripts(PlaceholderRegistry& reg);
    void registerDefaultPageTitle(PlaceholderRegistry& reg);
    void registerDefaultSubtitle(PlaceholderRegistry& reg);
    void registerDefaultMenu(PlaceholderRegistry& reg);
    void registerDefaultStatus(PlaceholderRegistry& reg);
    void registerDefaultPlaceholders(PlaceholderRegistry& reg);
    void applyTemplateSetupCallback(PlaceholderRegistry& reg);
    
    // Rebuild registry with defaults and optional customizer
    void rebuildPlaceholderRegistry(std::function<void(PlaceholderRegistry&)> customizer = nullptr);
    
    // Create server instance
    void createServer(uint16_t port);
    
    // Register all routes (takes handler callbacks)
    void registerRoutes();
    
    // Setup DNS server for captive portal
    void setupDNSD();
    
    // Process DNS requests (call periodically)
    void processDNS();
    
    // Shutdown and cleanup server and DNS
    void shutdownServer();
    
    // Get server instance
    AsyncWebServer* getServer() { return server.get(); }
    
    // Get DNS server instance (for testing)
    DNSServer* getDNSServer() { return dnsServer.get(); }
    
  private:
    WiFiManager* _wm;
    std::unique_ptr<WiFiManagerHandlers> _handlers;
    std::unique_ptr<AsyncWebServer> server;
    std::unique_ptr<DNSServer> dnsServer;
    
    // Shared DFTE placeholder registry
    std::unique_ptr<PlaceholderRegistry> _tplRegistry;
    std::function<void(PlaceholderRegistry&)> _tplSetupCallback;
    
    // Singleton instance
    static WiFiManagerServer* s_instance;
};

// Include WiFiManagerHandlers.h after class definition to provide full definition
// needed for std::unique_ptr destructor
#include "WiFiManagerHandlers.h"

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerServer_h
