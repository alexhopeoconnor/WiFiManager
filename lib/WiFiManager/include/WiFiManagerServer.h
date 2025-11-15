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

// Forward declarations
class WiFiManager;

// Forward declaration - full definition needed in .cpp file
class WiFiManagerHandlers;

class WiFiManagerServer {
  public:
    WiFiManagerServer(WiFiManager* wm);
    
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
};

// Include WiFiManagerHandlers.h after class definition to provide full definition
// needed for std::unique_ptr destructor
#include "WiFiManagerHandlers.h"

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerServer_h
