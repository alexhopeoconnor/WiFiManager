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
#ifdef WM_DFTE_LOGGING
#include "WiFiManagerDfteLogger.h"
#include <DeviceFrameworkTemplateEngineDebug.h>
#endif
#include "templates/CSS.h"
#include "templates/JS.h"

#if defined(ESP8266) || defined(ESP32)

WiFiManagerServer* WiFiManagerServer::s_instance = nullptr;

WiFiManagerServer* WiFiManagerServer::instance() {
  return s_instance;
}

WiFiManagerServer::WiFiManagerServer(WiFiManager* wm) 
  : _wm(wm), _handlers(std::make_unique<WiFiManagerHandlers>(wm)) {
  s_instance = this;
}

WiFiManagerServer::~WiFiManagerServer() = default;

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

void WiFiManagerServer::registerDefaultStyles(PlaceholderRegistry& reg) {
  reg.registerProgmemData("%STYLES%", CSS_STYLE);
}

void WiFiManagerServer::registerDefaultScripts(PlaceholderRegistry& reg) {
  reg.registerProgmemData("%SCRIPTS%", JS_SCRIPT);
}

void WiFiManagerServer::registerDefaultPageTitle(PlaceholderRegistry& reg) {
  reg.registerRamData("%PAGE_TITLE%", &WiFiManagerServer::tplGetPageTitle);
}

void WiFiManagerServer::registerDefaultSubtitle(PlaceholderRegistry& reg) {
  reg.registerRamData("%SUBTITLE%", &WiFiManagerServer::tplGetSubtitle);
}

void WiFiManagerServer::registerDefaultMenu(PlaceholderRegistry& reg) {
  reg.registerRamData("%MENU%", &WiFiManagerServer::tplGetMenu);
}

void WiFiManagerServer::registerDefaultStatus(PlaceholderRegistry& reg) {
  reg.registerRamData("%STATUS%", &WiFiManagerServer::tplGetStatus);
}

void WiFiManagerServer::registerDefaultPlaceholders(PlaceholderRegistry& reg) {
  registerDefaultStyles(reg);
  registerDefaultScripts(reg);
  registerDefaultPageTitle(reg);
  registerDefaultSubtitle(reg);
  registerDefaultMenu(reg);
  registerDefaultStatus(reg);
}

void WiFiManagerServer::applyTemplateSetupCallback(PlaceholderRegistry& reg) {
  if (_tplSetupCallback) {
    _tplSetupCallback(reg);
  }
}

void WiFiManagerServer::rebuildPlaceholderRegistry(std::function<void(PlaceholderRegistry&)> customizer) {
  _tplRegistry = std::unique_ptr<PlaceholderRegistry>(new PlaceholderRegistry(WM_TEMPLATE_REGISTRY_CAPACITY));
  registerDefaultPlaceholders(*_tplRegistry);
  if (customizer) {
    customizer(*_tplRegistry);
  } else {
    applyTemplateSetupCallback(*_tplRegistry);
  }
}

void WiFiManagerServer::setupTemplateEngine() {
  // Initialize shared placeholder registry once
  if (!_tplRegistry) {
    _tplRegistry = std::unique_ptr<PlaceholderRegistry>(new PlaceholderRegistry(WM_TEMPLATE_REGISTRY_CAPACITY));
  } else {
    _tplRegistry->clear();
  }
  
  // Register all default placeholders
  registerDefaultPlaceholders(*_tplRegistry);
  
  // Allow consumers to customize placeholders
  applyTemplateSetupCallback(*_tplRegistry);

#ifdef WM_DFTE_LOGGING
  // Install DFTE log sink only if nothing else registered yet (e.g. DeviceFramework's bridge).
  if (!deviceFrameworkTemplateEngineIsLoggingEnabled()) {
    if (!_dfteLogger) {
      _dfteLogger = std::make_unique<WiFiManagerDfteLogger>(_wm);
    }
    deviceFrameworkTemplateEngineEnableLogging(_dfteLogger.get(), static_cast<const void*>(this));
    _wmOwnsDfteLogSink = true;
  }
#endif
}

void WiFiManagerServer::createServer(uint16_t port) {
  // If server already exists, shutdown first to prevent leaks and connection drops
  if (server) {
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("Server already exists, shutting down first"));
    #endif
    shutdownServer();
  }

  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Starting Web Portal"));
  #endif

  if(port != 80) {
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("http server started with custom port: "), port);
    #endif
  }

  server.reset(new AsyncWebServer(port));
  
  // Ensure template engine is ready whenever a server is (re)created
  setupTemplateEngine();
}

void WiFiManagerServer::registerRoutes() {
  if (!server) {
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] Server not created, call createServer() first"));
    #endif
    return;
  }

  // Ensure template engine is initialized before routes use it
  if (!_tplRegistry) {
    setupTemplateEngine();
  }
  
  if (_wm->_webservercallback != NULL) {
    #ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[CB] _webservercallback calling"));
    #endif
    _wm->_webservercallback(); // @CALLBACK
  }
  
  /* Setup httpd callbacks, web pages: root, wifi config pages, SO captive portal detectors and not found. */

  // Register routes with lambdas
  server->on(WM_G(R_root), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleRoot(request);
  });

  server->on(WM_G(R_api_bootstrap), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiBootstrap(request);
  });
  server->on(WM_G(R_api_wifi_scan_status), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiWifiScanStatus(request);
  });
  server->on(WM_G(R_api_wifi_scan), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiWifiScan(request);
  });
  server->on(WM_G(R_api_wifi_meta), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiWifiMeta(request);
  });
  server->on(WM_G(R_api_wifi_save), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiWifiSave(request);
  });
  server->on(WM_G(R_api_params), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiParamsGet(request);
  });
  server->on(WM_G(R_api_params_save), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiParamsSave(request);
  });
  server->on(WM_G(R_api_info), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiInfo(request);
  });
  server->on(WM_G(R_api_status), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiStatus(request);
  });
  server->on(WM_G(R_api_device_restart), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiDeviceRestart(request);
  });
  server->on(WM_G(R_api_device_erase), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiDeviceErase(request);
  });
  server->on(WM_G(R_api_portal_close), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiPortalClose(request);
  });
  server->on(WM_G(R_api_portal_exit), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiPortalExit(request);
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
  
  server->on(WM_G(R_scanstatus), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWiFiScanStatus(request);
  });

  server->on(WM_G(R_scan), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleWiFiScanRequest(request);
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
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("HTTP server started"));
  #endif
}

void WiFiManagerServer::setupDNSD() {
  dnsServer.reset(new DNSServer());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  #ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("dns server started with ip: "), WiFi.softAPIP());
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
#ifdef WM_DFTE_LOGGING
  if (_wmOwnsDfteLogSink) {
    deviceFrameworkTemplateEngineDisableLoggingForOwner(static_cast<const void*>(this));
    _dfteLogger.reset();
    _wmOwnsDfteLogSink = false;
  }
#endif

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

