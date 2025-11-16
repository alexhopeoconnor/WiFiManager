/**
 * WiFiManagerServer.cpp
 * 
 * Server lifecycle management implementation
 * 
 * @author tablatronix
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#include "WiFiManagerServer.h"
#include "WiFiManager.h"
#include "WiFiManagerHandlers.h" // Full definition needed for unique_ptr
#include "templates/CSS.h"
#include "templates/JS.h"
#include "templates/Root.h"

#if defined(ESP8266) || defined(ESP32)

WiFiManagerServer* WiFiManagerServer::s_instance = nullptr;

WiFiManagerServer* WiFiManagerServer::instance() {
  return s_instance;
}

WiFiManagerServer::WiFiManagerServer(WiFiManager* wm) 
  : _wm(wm), _handlers(std::make_unique<WiFiManagerHandlers>(wm)) {
  s_instance = this;
}

// Static getters for RAM placeholders (registry requires zero-arg functions) using singleton
const char* WiFiManagerServer::tplGetPageTitle() {
  auto srv = WiFiManagerServer::instance();
  if (!srv || !srv->_wm) return "";
  return srv->_wm->_title.c_str();
}

const char* WiFiManagerServer::tplGetSubtitle() {
  static String buf;
  auto srv = WiFiManagerServer::instance();
  if (!srv || !srv->_wm) { buf = ""; return buf.c_str(); }
  WiFiManager* wm = srv->_wm;
  if (wm->configPortalActive) {
    buf = wm->_apName;
  } else {
    buf = wm->getWiFiHostname() + " - " + WiFi.localIP().toString();
  }
  return buf.c_str();
}

const char* WiFiManagerServer::tplGetMenu() {
  static String buf;
  auto srv = WiFiManagerServer::instance();
  if (!srv || !srv->_handlers) { buf = ""; return buf.c_str(); }
  buf = "";
  srv->_handlers->getMenuOut(&buf);
  return buf.c_str();
}

const char* WiFiManagerServer::tplGetStatus() {
  static String buf;
  auto srv = WiFiManagerServer::instance();
  if (!srv || !srv->_handlers) { buf = ""; return buf.c_str(); }
  buf = "";
  srv->_handlers->reportStatus(buf);
  return buf.c_str();
}

void WiFiManagerServer::setupTemplateEngine() {
  // Initialize shared placeholder registry once
  if (!_tplRegistry) {
    _tplRegistry = std::unique_ptr<PlaceholderRegistry>(new PlaceholderRegistry(12));
  } else {
    _tplRegistry->clear();
  }
  
  // Register PROGMEM assets
  _tplRegistry->registerProgmemData("%STYLES%", CSS_STYLE);
  _tplRegistry->registerProgmemData("%SCRIPTS%", JS_SCRIPT);
  
  // Register dynamic RAM getters
  _tplRegistry->registerRamData("%PAGE_TITLE%", &WiFiManagerServer::tplGetPageTitle);
  _tplRegistry->registerRamData("%SUBTITLE%", &WiFiManagerServer::tplGetSubtitle);
  _tplRegistry->registerRamData("%MENU%", &WiFiManagerServer::tplGetMenu);
  _tplRegistry->registerRamData("%STATUS%", &WiFiManagerServer::tplGetStatus);
}

void WiFiManagerServer::createServer(uint16_t port) {
  // If server already exists, shutdown first to prevent leaks and connection drops
  if (server) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("Server already exists, shutting down first"));
    #endif
    shutdownServer();
  }

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(F("Starting Web Portal"));
  #endif

  if(port != 80) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("http server started with custom port: "), port);
    #endif
  }

  server.reset(new AsyncWebServer(port));
  
  // Ensure template engine is ready whenever a server is (re)created
  setupTemplateEngine();
}

void WiFiManagerServer::registerRoutes() {
  if (!server) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] Server not created, call createServer() first"));
    #endif
    return;
  }

  // Ensure template engine is initialized before routes use it
  if (!_tplRegistry) {
    setupTemplateEngine();
  }
  
  if (_wm->_webservercallback != NULL) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("[CB] _webservercallback calling"));
    #endif
    _wm->_webservercallback(); // @CALLBACK
  }
  
  /* Setup httpd callbacks, web pages: root, wifi config pages, SO captive portal detectors and not found. */

  // Register routes with lambdas
  server->on(WM_G(R_root), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleRoot(request);
  });
  
  server->on(WM_G(R_wifi), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWifi(request, true);
  });
  
  server->on(WM_G(R_wifinoscan), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWifi(request, false);
  });
  
  server->on(WM_G(R_wifisave), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWifiSave(request);
  });
  
  server->on(WM_G(R_info), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleInfo(request);
  });
  
  server->on(WM_G(R_param), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleParam(request);
  });
  
  server->on(WM_G(R_paramsave), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleParamSave(request);
  });
  
  server->on(WM_G(R_restart), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleReset(request);
  });
  
  server->on(WM_G(R_exit), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleExit(request);
  });
  
  server->on(WM_G(R_close), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleClose(request);
  });
  
  server->on(WM_G(R_erase), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleErase(request, false);
  });
  
  server->on(WM_G(R_status), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWiFiStatus(request);
  });
  
  server->on(WM_G(R_scanstatus), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWiFiScanStatus(request);
  });
  
  // OTA Update routes
  server->on(WM_G(R_update), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleUpdate(request);
  });
  
  server->on(WM_G(R_updatedone), HTTP_POST, 
    [this](AsyncWebServerRequest *request) {
      this->_handlers->handleUpdateDone(request);
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      this->_handlers->handleUpdating(request, filename, index, data, len, final);
    }
  );
  
  // 404 handler
  server->onNotFound([this](AsyncWebServerRequest *request) {
    this->_handlers->handleNotFound(request);
  });
  
  server->begin(); // Web server start
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("HTTP server started"));
  #endif
}

void WiFiManagerServer::setupDNSD() {
  dnsServer.reset(new DNSServer());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("dns server started with ip: "), WiFi.softAPIP());
  #endif
  const uint8_t DNS_PORT = 53;
  dnsServer->start(DNS_PORT, F("*"), WiFi.softAPIP());
}

void WiFiManagerServer::processDNS() {
  if (dnsServer) {
    dnsServer->processNextRequest();
  }
}

void WiFiManagerServer::shutdownServer() {
  // AsyncWebServer doesn't have stop(), just reset the unique_ptr to free resources
  if (server) {
    server.reset();
  }
  
  // Shutdown DNS server
  if (dnsServer) {
    dnsServer->stop(); // free heap ?
    dnsServer.reset();
  }
  
  // Clear singleton on shutdown
  if (s_instance == this) s_instance = nullptr;
}

#endif

