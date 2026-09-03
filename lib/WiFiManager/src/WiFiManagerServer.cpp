/**
 * WiFiManagerServer.cpp
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * HTTP server lifecycle and route registration only. HTML shell rendering is request-scoped in
 * WiFiManagerHandlers (WM_ROOT_SHELL_TEMPLATE + per-request placeholder registry).
 * Route inventory matches WiFiManagerServer.h (GET /, /api/..., POST /u, onNotFound).
 */

#include "WiFiManagerServer.h"
#include "WiFiManager.h"
#include "WiFiManagerHandlers.h"
#ifdef WM_DFTE_LOGGING
#include "WiFiManagerDfteLogger.h"
#include <DeviceFrameworkTemplateEngineDebug.h>
#endif

#if defined(ESP8266) || defined(ESP32)

WiFiManagerServer::WiFiManagerServer(WiFiManager* wm)
    : _wm(wm), _handlers(std::make_unique<WiFiManagerHandlers>(wm)) {}

WiFiManagerServer::~WiFiManagerServer() = default;

void WiFiManagerServer::createServer(uint16_t port) {
  if (server) {
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("Server already exists, shutting down first"));
#endif
    shutdownServer();
  }

#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Starting Web Portal"));
#endif

  if (port != 80) {
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("http server started with custom port: "), port);
#endif
  }

  server.reset(new AsyncWebServer(port));

#ifdef WM_DFTE_LOGGING
  // DFTE logging is still owned by server lifecycle; it is intentionally independent of
  // request-scoped shell rendering / placeholder registration.
  if (!deviceFrameworkTemplateEngineIsLoggingEnabled()) {
    if (!_dfteLogger) {
      _dfteLogger = std::make_unique<WiFiManagerDfteLogger>(_wm);
    }
    deviceFrameworkTemplateEngineEnableLogging(_dfteLogger.get(), static_cast<const void*>(this));
    _wmOwnsDfteLogSink = true;
  }
#endif
}

void WiFiManagerServer::registerRoutes() {
  if (!server) {
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] Server not created, call createServer() first"));
#endif
    return;
  }

  if (_wm->_webservercallback != NULL) {
#ifndef WM_NO_LOG
    _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[CB] _webservercallback calling"));
#endif
    _wm->_webservercallback();
  }

  // Shell: only HTML document. JSON + actions + OTA below (no legacy /wifi, /info, … routes).

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
  server->on(WM_G(R_api_wifi_connect_status), HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiWifiConnectStatus(request);
  });
  server->on(WM_G(R_api_wifi_connect_complete), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiWifiConnectComplete(request);
  });
  server->on(WM_G(R_api_portal_timeout_reset), HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->_handlers->handleApiPortalTimeoutReset(request);
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

  server->on(WM_G(R_updatedone), HTTP_POST,
              [this](AsyncWebServerRequest *request) {
                this->_handlers->handleUpdateDone(request);
              },
              [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
                     bool final) {
                this->_handlers->handleUpdating(request, filename, index, data, len, final);
              });

  server->onNotFound([this](AsyncWebServerRequest *request) {
    this->_handlers->handleNotFound(request);
  });

  server->begin();
#ifndef WM_NO_LOG
  _wm->log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("HTTP server started"));
#endif
}

void WiFiManagerServer::setupDNSD() {
  dnsServer.reset(new DNSServer());
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

  if (server) {
    server.reset();
  }

  if (dnsServer) {
    dnsServer->stop();
    dnsServer.reset();
  }
}

#endif
