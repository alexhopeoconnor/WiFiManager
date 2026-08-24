/**
 * WiFiManager.cpp
 * 
 * WiFiManager, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 * 
 * @author Creator tzapu
 * @author tablatronix
 * @author alexhopeoconnor
 * @license MIT
 */

#include "WiFiManager.h"
#include "WiFiManagerLogSink.h"
#include "WiFiManagerServer.h"

#if defined(ESP8266) || defined(ESP32)

#ifdef ESP32
#include <esp_heap_caps.h>
#endif

#ifdef ESP32
uint8_t WiFiManager::_lastconxresulttmp = WL_IDLE_STATUS;
#endif

namespace {
constexpr size_t kMaxHostnameLength = 32;
constexpr uint8_t kSoftApStartMaxAttempts = 3;

bool isValidHostnameChar(char c) {
  return isAlphaNumeric(c) || c == '-';
}

bool normalizeHostname(String& hostname) {
  hostname.trim();

  if (hostname.length() > kMaxHostnameLength) {
    return false;
  }

  if (hostname.length() == 0) {
    return true;
  }

  if (hostname[0] == '-' || hostname[hostname.length() - 1] == '-') {
    return false;
  }

  for (size_t i = 0; i < hostname.length(); i++) {
    if (!isValidHostnameChar(hostname[i])) {
      return false;
    }
  }

  return true;
}
} // namespace

/**
 * Add a custom parameter to the config portal
 * @param p Pointer to WiFiManagerParameter to add
 * @return true if added successfully, false on error
 */
bool WiFiManager::portalAddParameter(WiFiManagerParameter *p) {
  if (!p) {
    return false;
  }

  // check param id is valid, unless null
  if(p->getID()){
    for (size_t i = 0; i < strlen(p->getID()); i++){
       if(!(isAlphaNumeric(p->getID()[i])) && !(p->getID()[i]=='_')){
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] parameter IDs can only contain alpha numeric chars"));
        #endif
        return false;
       }
    }
  }

  // init params if never malloc
  if(_params == NULL){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("allocating params bytes:"),_max_params * sizeof(WiFiManagerParameter*));        
    #endif
    _params = (WiFiManagerParameter**)malloc(_max_params * sizeof(WiFiManagerParameter*));
    if (_params == NULL) {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] failed to allocate params"));
      #endif
      return false;
    }
  }

  // Resize only after a successful allocation so _max_params always matches storage.
  if(_paramsCount == _max_params){
    const size_t expandedMaxParams = _max_params + WIFI_MANAGER_MAX_PARAMS;
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("Updated _max_params:"),expandedMaxParams);
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("re-allocating params bytes:"),expandedMaxParams * sizeof(WiFiManagerParameter*));
    #endif
    WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_params, expandedMaxParams * sizeof(WiFiManagerParameter*));
    if (new_params != NULL) {
      _params = new_params;
      _max_params = expandedMaxParams;
    } else {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] failed to realloc params, size not increased!"));
      #endif
      return false;
    }
  }

  _params[_paramsCount] = p;
  _paramsCount++;
  
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Added Parameter:"),p->getID());
  #endif
  return true;
}

void WiFiManager::portalClearParameters() {
  if (_params != NULL) {
    free(_params);
    _params = NULL;
  }
  _paramsCount = 0;
  _max_params = WIFI_MANAGER_MAX_PARAMS;
}

WiFiManagerParameter** WiFiManager::getParameters() {
  return _params;
}

int WiFiManager::getParametersCount() {
  return _paramsCount;
}

// Constructors
WiFiManager::WiFiManager(Print& consolePort) : _logPort(consolePort) {
  WiFiManagerInit();
}

WiFiManager::WiFiManager() {
  WiFiManagerInit();  
}

void WiFiManager::WiFiManagerInit(){
  if (_logEnabled && (uint8_t)_runtimeMaxLevel >= (uint8_t)WiFiManagerLogLevel::Trace) {
    debugPlatformInfo();
  }
  _max_params = WIFI_MANAGER_MAX_PARAMS;
  // _serverManager is created lazily when config portal is started to save memory
}

// destructor
WiFiManager::~WiFiManager() {
  // Cleanup server if still active (prevents leaks if WiFiManager is destroyed while portal is running)
  if (_serverManager) {
    _serverManager->shutdownServer();
  }
  
  _end();
  
  // Free allocated parameters
  if (_params != NULL){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("freeing allocated params!"));
    #endif
    free(_params);
    _params = NULL;
  }

  // Remove ESP32 event handler
  #ifdef ESP32
    WiFi.removeEvent(wm_event_id);
  #endif

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("unloading"));
  #endif
}

void WiFiManager::_begin(){
  if(_hasBegun) return;
  _hasBegun = true;

  #ifndef ESP32
  WiFi.persistent(false); // disable persistent so scannetworks and mode switching do not cause overwrites
  #endif
}

void WiFiManager::_end(){
  _hasBegun = false;
  if(_userpersistent) WiFi.persistent(true); // reenable persistent, there is no getter we rely on _userpersistent
}

boolean WiFiManager::autoConnect() {
  String ssid = getDefaultAPName();
  return autoConnect(ssid.c_str(), NULL);
}

/**
 * Auto-connect to saved WiFi credentials, or start config portal on failure
 * @param apName Access point name for config portal (if connection fails)
 * @param apPassword Optional access point password
 * @return true if connected successfully, false if config portal started
 */
boolean WiFiManager::autoConnect(char const *apName, char const *apPassword) {
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("AutoConnect"));
  #endif

  #ifdef ESP32
  setupHostname(true);

  if(_hostname != ""){
    // disable wifi if already on
    if(WiFi.getMode() & WIFI_STA){
      WiFi.mode(WIFI_OFF);
      int timeout = millis()+1200;
      // async loop for mode change
      while(WiFi.getMode()!= WIFI_OFF && millis()<timeout){
        delay(0);
      }
    }
  }
  #endif

  // Try the startup STA path first; only fall back to the config portal after that workflow finishes.
  _startconn = millis();
  _begin();
  const wm_autoconnect_result_t autoConnectResult = attemptAutoConnect();
  if(autoConnectResult == wm_autoconnect_result_t::connected){
    return true; // connected success
  }
  if(autoConnectResult == wm_autoconnect_result_t::fatal){
    return false;
  }

  #ifndef WM_NO_LOG
  if(autoConnectResult == wm_autoconnect_result_t::no_credentials){
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("No Credentials are Saved, skipping connect"));
  } else {
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("AutoConnect: FAILED for "),(String)((millis()-_startconn)) + " ms");
  }
  #endif

  // possibly skip the config portal
  if (!_enableConfigPortal) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("enableConfigPortal: FALSE, skipping "));
    #endif

    return false; // not connected and not cp
  }

  // not connected start configportal
  startConfigPortal(apName, apPassword);
  return false; // Config portal started, but not connected yet
}

WiFiManager::wm_autoconnect_result_t WiFiManager::attemptAutoConnect() {
  // Attempt the startup STA path only; portal fallback is handled by autoConnect().
  if(!WiFi.enableSTA(true)){
    // handle failure mode Brownout detector etc.
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[FATAL] Unable to enable wifi!"));
    #endif
    return wm_autoconnect_result_t::fatal;
  }
  
  WiFiSetCountry();

  #ifdef ESP32
  if(esp32persistent) WiFi.persistent(false); // disable persistent for esp32 after esp_wifi_start or else saves wont work
  #endif

  _usermode = WIFI_STA; // When using autoconnect , assume the user wants sta mode on permanently.

  // no getter for autoreconnectpolicy before this
  // https://github.com/esp8266/Arduino/pull/4359
  // so we must force it on else, if not connectimeout then waitforconnectionresult gets stuck endless loop
  WiFi_autoReconnect();

  #ifdef ESP8266
  if(_hostname != ""){
    setupHostname(true);
  }
  #endif

  // Check if already connected, otherwise try stored credentials
  bool connected = false;
  if (WiFi.status() == WL_CONNECTED){
    connected = true;
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("AutoConnect: ESP Already Connected"));
    #endif
    setSTAConfig();
  }

  const bool hasSavedCredentials = _defaultssid != "" || WiFi_hasAutoConnect();
  if (!connected && !hasSavedCredentials) {
    return wm_autoconnect_result_t::no_credentials;
  }

  if(connected || connectWifi(_defaultssid, _defaultpass) == WL_CONNECTED){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("AutoConnect: SUCCESS"));
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Connected in"),(String)((millis()-_startconn)) + " ms");
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("STA IP Address:"),WiFi.localIP());
    #endif
    _lastconxresult = WL_CONNECTED;

    if(_hostname != ""){
      #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("hostname: STA: "),getWiFiHostname());
      #endif
    }
    return wm_autoconnect_result_t::connected;
  }

  return wm_autoconnect_result_t::failed;
}

bool WiFiManager::setupHostname(bool restart){
  if(_hostname == "") {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("No Hostname to set"));
    #endif
    return false;
  } 
  else {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Setting Hostnames: "),_hostname);
    #endif
  }
  bool res = true;
  #ifdef ESP8266
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Setting WiFi hostname"));
    #endif
    res = WiFi.hostname(_hostname.c_str());
    #ifdef WM_MDNS
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Setting MDNS hostname, tcp 80"));
      #endif
      if(MDNS.begin(_hostname.c_str())){
        MDNS.addService("http", "tcp", 80);
      }
    #endif
  #elif defined(ESP32)
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Setting WiFi hostname"));
    #endif

    res = WiFi.setHostname(_hostname.c_str());
    #ifdef WM_MDNS
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Setting MDNS hostname, tcp 80"));
        #endif
      if(MDNS.begin(_hostname.c_str())){
        MDNS.addService("http", "tcp", 80);
      }
    #endif
  #endif

  #ifndef WM_NO_LOG
  if(!res)log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] hostname: set failed!"));
  #endif

  if(restart && (WiFi.status() == WL_CONNECTED)){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("reconnecting to set new hostname"));
    #endif
    WiFi_Disconnect();
    delay(200); // do not remove, need a delay for disconnect to change status()
  }

  return res;
}

bool WiFiManager::startAP(){
  bool ret = true;
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("StartAP with SSID: "),_apName);
  #endif

  #ifdef ESP8266
    if(!WiFi.enableAP(true)) {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] enableAP failed!"));
      #endif
      return false;
    }
    delay(500); // workaround delay
  #endif

  // setup optional soft AP static ip config
  if (_ap_static_ip) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Custom AP IP/GW/Subnet:"));
    #endif
    if(!WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn)){
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] softAPConfig failed!"));
      #endif
    }
  }

  // TODO: Revisit channel sync when STA is disconnected; consider explicit default vs sync
  int32_t channel = 0;
  if(_channelSync) channel = WiFi.channel();
  else channel = _apChannel;

  if(channel>0){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Starting AP on channel:"),channel);
    #endif
  }

  uint8_t apChannel = channel > 0 ? channel : 1;
  auto startSoftAP = [&]() -> bool {
    if (_apPassword != "") {
      return WiFi.softAP(_apName.c_str(), _apPassword.c_str(), apChannel, _apHidden);
    }

    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("AP has anonymous access!"));
    #endif
    return WiFi.softAP(_apName.c_str(), "", apChannel, _apHidden);
  };

  // Start soft AP with password or anonymous
  ret = startSoftAP();
  for (uint8_t attempt = 1; !ret && attempt < kSoftApStartMaxAttempts; attempt++) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] softAP start failed, retry attempt"), attempt + 1);
    #endif
    WiFi.softAPdisconnect(false);
    delay(150 * attempt);
    ret = startSoftAP();
  }

  if ((uint8_t)_runtimeMaxLevel >= (uint8_t)WiFiManagerLogLevel::Trace) {
    debugSoftAPConfig();
  }

  delay(500); // slight delay to make sure we get an AP IP
  #ifndef WM_NO_LOG
  if(!ret) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] There was a problem starting the AP"));
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("AP IP address:"),WiFi.softAPIP());
  #endif

  // set ap hostname
  #ifdef ESP32
    if(ret && _hostname != ""){
      bool res =  WiFi.softAPsetHostname(_hostname.c_str());
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("setting softAP Hostname:"),_hostname);
      if(!res)log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] hostname: AP set failed!"));
      log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("hostname: AP: "),WiFi.softAPgetHostname());
      #endif
   }
  #endif

  return ret;
}

/**
 * Start web portal (allows reconfiguration without disconnecting STA)
 */
void WiFiManager::startWebPortal() {
  if(configPortalActive || webPortalActive) return;
  abort = false;
  setupConfigPortal();
  webPortalActive = true;
}

/**
 * Stop web portal
 */
void WiFiManager::stopWebPortal() {
  if(!configPortalActive && !webPortalActive) return;
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Stopping Web Portal"));  
  #endif
  webPortalActive = false;
  shutdownConfigPortal();
}

boolean WiFiManager::configPortalHasTimeout(){
    if(!configPortalActive) return false;
    uint16_t logintvl = 30000; // how often to emit timeing out counter logging

    // handle timeout portal client check
    if(_configPortalTimeout == 0 || (_apClientCheck && (WiFi_softap_num_stations() > 0))){
      // debug num clients every 30s
      if(millis() - timer > logintvl){
        timer = millis();
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("NUM CLIENTS: "),(String)WiFi_softap_num_stations());
        #endif
      }
      _configPortalStart = millis(); // kludge, bump configportal start time to skew timeouts
      return false;
    }

    // handle timeout webclient check
    if(_webClientCheck && (_webPortalAccessed>_configPortalStart)>0) _configPortalStart = _webPortalAccessed;

    // handle timed out
    if(millis() > _configPortalStart + _configPortalTimeout){
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("config portal has timed out"));
      #endif
      return true; // timeout bail, else do debug logging
    } 
    else if (_logEnabled && (uint8_t)_runtimeMaxLevel > (uint8_t)WiFiManagerLogLevel::Silent) {
      // log timeout time remaining every 30s
      if((millis() - timer) > logintvl){
        timer = millis();
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Portal Timeout In"),(String)((_configPortalStart + _configPortalTimeout-millis())/1000) + (String)F(" seconds"));
        #endif
      }
    }

    return false;
}

void WiFiManager::setupConfigPortal() {
  // Lazy initialization: only create server manager when config portal is actually needed
  if (!_serverManager) {
    _serverManager = std::make_unique<WiFiManagerServer>(this);
  }
  _serverManager->createServer(_httpPort);
  _serverManager->registerRoutes();
  resetAsyncScan(true);
  if(_preloadwifiscan) scheduleScan(WM_SCAN_SCHEDULE_PRELOAD); // process() starts it later
}

void WiFiManager::startConfigPortal() {
  String ssid = getDefaultAPName();
  startConfigPortal(ssid.c_str(), NULL);
}

/**
 * Start config portal (AP mode)
 * @param apName Access point name
 * @param apPassword Optional access point password
 */
void WiFiManager::startConfigPortal(char const *apName, char const *apPassword) {
  _begin();

  if(configPortalActive){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Starting Config Portal FAILED, is already running"));
    #endif    
    return;
  }

  // Setup AP
  _apName     = apName;
  _apPassword = apPassword;
  
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Starting Config Portal"));
  #endif

  if(_apName == "") _apName = getDefaultAPName();

  if(!validApPassword()) return;
  
  // Handle issues with STA connections - shutdown STA if not connected to avoid hanging channel scanning
  if(_disableSTA || (!WiFi.isConnected() && _disableSTAConn)){
    #ifdef WM_DISCONWORKAROUND
      WiFi.mode(WIFI_AP_STA);
    #endif
    WiFi_Disconnect();
    WiFi_enableSTA(false);
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Disabling STA"));
    #endif
  }

  // init configportal globals to known states
  configPortalActive = true;
  abort = false;
  _hasEnteredConfigPortal = true;
  _cpConnectState = wm_cp_connect_state_t::idle;
  _cpConnectMessage = F("Config portal active");
  _scanLifecycleBlocked = true;
  emitPortalEvent(WM_EVENT_PORTAL_STARTED);

  _configPortalStart = millis();

  // start access point
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Enabling AP"));
  #endif
  startAP();
  WiFiSetCountry();

  // do AP callback if set
  if ( _apcallback != NULL) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("[CB] _apcallback calling"));
    #endif
    _apcallback(this);
  }

  // init configportal
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("setupConfigPortal"));
  #endif
  setupConfigPortal();

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("setupDNSD"));
  #endif  
  if (_serverManager) {
    _serverManager->setupDNSD();
  }
  _scanLifecycleBlocked = false;

  #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Config Portal Running (call process() periodically)"));
    if(_configPortalTimeout > 0) log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Portal Timeout In"),(String)(_configPortalTimeout/1000) + (String)F(" seconds"));
  #endif
}

/**
 * Process config portal/web portal (must be called periodically)
 * @return true if connected successfully, false otherwise
 */
boolean WiFiManager::process(){
    // Check for scheduled reboots
    if (_rebootScheduled && millis() >= _rebootTime) {
      _rebootScheduled = false;
      reboot();
      return false;
    }
    
    // Check for scheduled aborts
    if (_abortScheduled && millis() >= _abortTime) {
      _abortScheduled = false;
      abort = true;
    }
    
    // process mdns, esp32 not required
    #if defined(WM_MDNS) && defined(ESP8266)
    MDNS.update();
    #endif
    
    processScan();
    processPortalConnect();

    if(webPortalActive || configPortalActive){
      // if timed out or abort, break
      if(_allowExit && (configPortalHasTimeout() || abort)){
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, F("process loop abort"));
        #endif
        webPortalActive = false;
        shutdownConfigPortal();
        if (_configportaltimeoutcallback != NULL) {
          #ifndef WM_NO_LOG
          log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[CB] config portal timeout callback"));
          #endif
          _configportaltimeoutcallback();  // @CALLBACK
        }
        return false;
      }

      processConfigPortal();
      return false;
    }
    return false;
}

void WiFiManager::emitPortalEvent(wm_event_t event) {
  if (_eventCallback) {
    _eventCallback(event);
  }
}

void WiFiManager::queuePortalConnect(const String& ssid, const String& pass) {
  _cpConnectSsid = ssid;
  _cpConnectPass = pass;
  _cpConnectStatus = WL_IDLE_STATUS;
  _cpConnectQueuedAt = millis();
  _cpConnectStartedAt = 0;
  _cpConnectDelayUntil = 0;
  _cpConnectCloseAt = 0;
  _cpConnectTimeoutMs = (_saveTimeout > 0) ? (unsigned long)_saveTimeout : _connectTimeout;
  _cpConnectStationIp = "";
  _cpConnectState = wm_cp_connect_state_t::queued;
  _cpConnectMessage = ssid.length() ? F("WiFi connect queued") : F("Settings saved");
  emitPortalEvent(WM_EVENT_PORTAL_CONNECT_QUEUED);
}

void WiFiManager::processPortalConnect() {
  const unsigned long now = millis();
  const unsigned long successHandoffDelayMs = 2500UL;
  auto finalizePortalConnectSuccess = [&]() {
    _cpConnectState = wm_cp_connect_state_t::success;
    if (_savewificallback != NULL) {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[CB] _savewificallback calling"));
      #endif
      _savewificallback();
    }
    emitPortalEvent(WM_EVENT_PORTAL_CONNECT_SUCCESS);
    if (_disableConfigPortal) {
      shutdownConfigPortal();
    }
  };
  auto failPortalConnect = [&](uint8_t status, const String& message) {
    _cpConnectStatus = status;
    _cpConnectMessage = message;
    _cpConnectStationIp = "";
    _cpConnectState = wm_cp_connect_state_t::failed;
    updateConxResult(status);
    if (_shouldBreakAfterConfig) {
      if (_savewificallback != NULL) {
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[CB] _savewificallback calling"));
        #endif
        _savewificallback();
      }
      shutdownConfigPortal();
    }
    emitPortalEvent(WM_EVENT_PORTAL_CONNECT_FAILED);
  };

  switch (_cpConnectState) {
    case wm_cp_connect_state_t::idle:
    case wm_cp_connect_state_t::failed:
      return;
    case wm_cp_connect_state_t::success:
      return;
    case wm_cp_connect_state_t::queued:
      resetAsyncScan(false);
      if (_cpConnectSsid.length() == 0) {
        _cpConnectStatus = WL_IDLE_STATUS;
        _cpConnectMessage = F("Settings saved");
        _cpConnectStationIp = "";
        _cpConnectState = wm_cp_connect_state_t::success;
        emitPortalEvent(WM_EVENT_PORTAL_CONNECT_SUCCESS);
        return;
      }
      _cpConnectDelayUntil =
        (_enableCaptivePortal && _cpclosedelay > 0) ? (now + (unsigned long)_cpclosedelay) : now;
      _cpConnectMessage = F("Preparing WiFi connect");
      _cpConnectState = wm_cp_connect_state_t::delaying;
      return;
    case wm_cp_connect_state_t::delaying:
      if (now < _cpConnectDelayUntil) {
        return;
      }
      _cpConnectState = wm_cp_connect_state_t::starting;
      return;
    case wm_cp_connect_state_t::starting: {
      setSTAConfig();
      if (_cleanConnect) {
        WiFi_Disconnect(); // match connectWifi(): force a clean STA begin for portal saves too
      }
      if (!_connectonsave) {
        if (!wifiConnectNew(_cpConnectSsid, _cpConnectPass, false)) {
          failPortalConnect(WL_CONNECT_FAILED, F("WiFi begin failed"));
          return;
        }
        _cpConnectStatus = WL_IDLE_STATUS;
        _cpConnectMessage = F("Settings saved");
        _cpConnectStationIp = "";
        _cpConnectState = wm_cp_connect_state_t::success;
        if (_savewificallback != NULL) {
          #ifndef WM_NO_LOG
          log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem, F("[CB] _savewificallback calling"));
          #endif
          _savewificallback();
        }
        emitPortalEvent(WM_EVENT_PORTAL_CONNECT_SUCCESS);
        return;
      }
      if (!wifiConnectNew(_cpConnectSsid, _cpConnectPass, _connectonsave)) {
        failPortalConnect(WL_CONNECT_FAILED, F("WiFi begin failed"));
        return;
      }
      _cpConnectStartedAt = now;
      _cpConnectMessage = F("Connecting to WiFi");
      _cpConnectState = wm_cp_connect_state_t::waiting;
      emitPortalEvent(WM_EVENT_PORTAL_CONNECT_START);
      return;
    }
    case wm_cp_connect_state_t::waiting: {
      const uint8_t status = WiFi.status();
      _cpConnectStatus = status;
      if (status == WL_CONNECTED) {
        updateConxResult(status);
        _cpConnectStationIp = WiFi.localIP().toString();
        _cpConnectMessage = _cpConnectStationIp.length() > 0
            ? String(F("WiFi connected. Redirecting to ")) + _cpConnectStationIp
            : String(F("WiFi connected"));
        if (_disableConfigPortal && (configPortalActive || webPortalActive)) {
          _cpConnectCloseAt = now + successHandoffDelayMs;
          _cpConnectState = wm_cp_connect_state_t::success_waiting_close;
        } else {
          finalizePortalConnectSuccess();
        }
        return;
      }
      if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || status == WL_CONNECTION_LOST) {
        failPortalConnect(status, getWLStatusString(status));
        return;
      }
      if (_cpConnectTimeoutMs > 0U && (now - _cpConnectStartedAt) >= _cpConnectTimeoutMs) {
        failPortalConnect(WL_CONNECT_FAILED, F("WiFi connect timeout"));
      }
      return;
    }
    case wm_cp_connect_state_t::success_waiting_close:
      if (now >= _cpConnectCloseAt) {
        finalizePortalConnectSuccess();
      }
      return;
  }
}

/**
 * Config portal maintenance (DNS). WiFi connect workflow is in processPortalConnect().
 * @return legacy status; connect result is reported via getConfigPortalConnectState().
 */
uint8_t WiFiManager::processConfigPortal(){
    if(configPortalActive && _serverManager){
      _serverManager->processDNS();
    }

    return WL_IDLE_STATUS;
}

/**
 * Shutdown config portal
 * @return true if shutdown successful, false otherwise
 */
bool WiFiManager::shutdownConfigPortal(){
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("shutdownConfigPortal"));
  #endif

  if(webPortalActive) return false;

  if(configPortalActive && _serverManager){
    _serverManager->processDNS();
  }
  
  // Shutdown server via server module
  if (_serverManager) {
    _serverManager->shutdownServer();
    // Free the server manager to reclaim memory (will be recreated lazily if needed)
    _serverManager.reset();
  }

  resetAsyncScan(true);

  if(!configPortalActive) return false;

  if (_cpConnectState == wm_cp_connect_state_t::queued || _cpConnectState == wm_cp_connect_state_t::delaying
      || _cpConnectState == wm_cp_connect_state_t::starting || _cpConnectState == wm_cp_connect_state_t::waiting) {
    _cpConnectState = wm_cp_connect_state_t::idle;
    _cpConnectMessage = F("Portal closed");
  }

  _scanLifecycleBlocked = true;

  // Turn off AP
  bool ret = false;
  ret = WiFi.softAPdisconnect(false);
  
  #ifndef WM_NO_LOG
  if(!ret)log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] disconnect configportal - softAPdisconnect FAILED"));
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("restoring usermode"),getModeString(_usermode));
  #endif
  delay(1000);
  WiFi_Mode(_usermode); // restore users wifi mode
  if(WiFi.status()==WL_IDLE_STATUS){
    WiFi.reconnect(); // restart wifi since we disconnected it in startconfigportal
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("WiFi Reconnect, was idle"));
    #endif
  }
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("wifi status:"),getWLStatusString(WiFi.status()));
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("wifi mode:"),getModeString(WiFi.getMode()));
  #endif
  configPortalActive = false;
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("configportal closed"));
  _end();
  _scanLifecycleBlocked = false;
  emitPortalEvent(WM_EVENT_PORTAL_STOPPED);
  return ret;
}

// TODO: Consider splitting saved vs new connect paths into separate methods
// (e.g., connectSaved() and connectNew()) to reduce branching and improve testability.
uint8_t WiFiManager::connectWifi(String ssid, String pass, bool connect) {
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Connecting as wifi client..."));
  #endif
  uint8_t retry = 1;
  uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

  setSTAConfig();
  
  // Make sure STA is on before `begin` so it does not call enableSTA->mode while persistent is ON (which would save WM AP state to eeprom)
  if(_cleanConnect) WiFi_Disconnect(); // disconnect before begin, in case anything is hung, this causes a 2 seconds delay for connect

  // if retry without delay (via begin()), the IDF is still busy even after returning status
  // E (5130) wifi:sta is connecting, return error
  // [E][WiFiSTA.cpp:221] begin(): connect failed!

  while(retry <= _connectRetries && (connRes!=WL_CONNECTED)){
  if(_connectRetries > 1){
    if(_aggresiveReconn) delay(1000); // add idle time before recon
    #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Connect Wifi, ATTEMPT #"),(String)retry+" of "+(String)_connectRetries); 
      #endif
  }
  // If SSID argument provided, connect to that (also handles preload() _defaultssid)
  if (ssid != "") {
    wifiConnectNew(ssid,pass,connect);
    if(_saveTimeout > 0){
      connRes = waitForConnectResult(_saveTimeout); // use default save timeout for saves to prevent bugs in esp->waitforconnectresult loop
    }
    else {
      connRes = waitForConnectResult();
    }
  }
  else {
    // connect using saved ssid if there is one
    if (WiFi_hasAutoConnect()) {
      wifiConnectDefault();
      connRes = waitForConnectResult();
    }
    else {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("No wifi saved, skipping"));
      #endif
    }
  }

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Connection result:"),getWLStatusString(connRes));
  #endif
  retry++;
}

// WPS enabled? https://github.com/esp8266/Arduino/pull/4889
#ifdef NO_EXTRA_4K_HEAP
  // Do WPS, if WPS options enabled and not connected and no password was supplied
  if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
    startWPS();
    // should be connected at the end of WPS
    connRes = waitForConnectResult();
  }
#endif

  if(connRes != WL_SCAN_COMPLETED){
    updateConxResult(connRes);
  }

  return connRes;
}

/**
 * Connect to a new WiFi access point
 * @param ssid SSID to connect to
 * @param pass Password for the access point
 * @param connect If true, connect immediately; if false, only save credentials
 * @return true if begin() succeeded, false otherwise
 */
bool WiFiManager::wifiConnectNew(String ssid, String pass,bool connect){
  bool ret = false;
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Connecting to NEW AP:"),ssid);
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("Using Password:"),pass);
  #endif
  WiFi_enableSTA(true,storeSTAmode); // storeSTAmode will also toggle STA on in default opmode (persistent) if true (default)
  WiFi.persistent(true);
  ret = WiFi.begin(ssid.c_str(), pass.c_str(), 0, NULL, connect);
  WiFi.persistent(false);
  #ifndef WM_NO_LOG
  if(!ret) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] wifi begin failed"));
  #endif
  return ret;
}

/**
 * Connect to stored WiFi credentials
 * @return true if begin() succeeded, false otherwise
 */
bool WiFiManager::wifiConnectDefault(){
  bool ret = false;

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Connecting to SAVED AP:"),WiFi_SSID(true));
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("Using Password:"),WiFi_psk(true));
  #endif

  ret = WiFi_enableSTA(true,storeSTAmode);
  delay(500); // Required delay for ESP8266 mode change to stabilize

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("Mode after delay: "),getModeString(WiFi.getMode()));
  if(!ret) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] wifi enableSta failed"));
  #endif

  ret = WiFi.begin();

  #ifndef WM_NO_LOG
  if(!ret) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] wifi begin failed"));
  #endif

  return ret;
}


/**
 * Set STA static IP configuration if configured
 * @return true if config succeeded or no config needed, false on error
 */
bool WiFiManager::setSTAConfig(){
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("STA static IP:"),_sta_static_ip);  
  #endif
  bool ret = true;
  if (_sta_static_ip) {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Custom static IP/GW/Subnet/DNS"));
      #endif
    if(_sta_static_dns) {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Custom static DNS"));
      #endif
      ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns);
    }
    else {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Custom STA IP/GW/Subnet"));
      #endif
      ret = WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
    }

      #ifndef WM_NO_LOG
      if(!ret) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] wifi config failed"));
      else log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("STA IP set:"),WiFi.localIP());
      #endif
  } 
  else {
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("setSTAConfig static ip not set, skipping"));
      #endif
  }
  return ret;
}

void WiFiManager::updateConxResult(uint8_t status){
  // Update connection result with wrong password detection
  _lastconxresult = status;
    #ifdef ESP8266
      if(_lastconxresult == WL_CONNECT_FAILED){
        if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD){
          _lastconxresult = WL_STATION_WRONG_PASSWORD;
        }
      }
    #elif defined(ESP32)
      if(_lastconxresult == WL_CONNECT_FAILED || _lastconxresult == WL_DISCONNECTED){
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("lastconxresulttmp:"),getWLStatusString(_lastconxresulttmp));            
        #endif
        if(_lastconxresulttmp != WL_IDLE_STATUS){
          _lastconxresult = _lastconxresulttmp;
        }
      }
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("lastconxresult:"),getWLStatusString(_lastconxresult));
    #endif
}

 
uint8_t WiFiManager::waitForConnectResult() {
  #ifndef WM_NO_LOG
  if(_connectTimeout > 0) log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,_connectTimeout,F("ms connectTimeout set")); 
  #endif
  return waitForConnectResult(_connectTimeout);
}

/**
 * waitForConnectResult
 * @param  uint16_t timeout  in seconds
 * @return uint8_t  WL Status
 */
uint8_t WiFiManager::waitForConnectResult(uint32_t timeout) {
  if (timeout == 0){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("connectTimeout not set, ESP waitForConnectResult..."));
    #endif
    return WiFi.waitForConnectResult();
  }

  unsigned long timeoutmillis = millis() + timeout;
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,timeout,F("ms timeout, waiting for connect..."));
  #endif
  uint8_t status = WiFi.status();
  
  while(millis() < timeoutmillis) {
    status = WiFi.status();
    if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
      return status;
    }
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("."));
    #endif
    delay(100);
  }
  return status;
}

// WPS enabled? https://github.com/esp8266/Arduino/pull/4889
#ifdef NO_EXTRA_4K_HEAP
void WiFiManager::startWPS() {
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("START WPS"));
  #endif
  #ifdef ESP8266  
    WiFi.beginWPSConfig();
  #endif
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("END WPS"));
  #endif
}
#endif

void WiFiManager::WiFi_scanComplete(int networksFound){
  if (_scan.state != WM_SCAN_RUNNING) {
    return;
  }
  _scan.completionPending = true;
  _scan.completionResult = networksFound;
  _scan.completionGeneration = _scan.runningGeneration;
}

bool WiFiManager::WiFi_scanNetworks(){
  return WiFi_scanNetworks(false);
}
 
bool WiFiManager::WiFi_scanNetworks(unsigned int cachetime){
    if (hasFreshScanResults(cachetime)) {
      return true;
    }
    scheduleScan(WM_SCAN_SCHEDULE_STALE_CACHE);
    return false;
}

bool WiFiManager::WiFi_scanNetworks(bool force){
    if(_numNetworks == 0 && _autoforcerescan){
      log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,"NO APs found forcing new scan");
      force = true;
    }

    if(!force && hasFreshScanResults(_scancachetime)){
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Scan is cached"),(String)(millis()-_lastscan )+" ms ago");
      #endif
      return true;
    }

    if (force) {
      scheduleScan(WM_SCAN_SCHEDULE_USER_REFRESH, true);
    } else {
      scheduleScan(WM_SCAN_SCHEDULE_STALE_CACHE);
    }
    return false;
}

void WiFiManager::requestAsyncScan(bool forceRefresh) {
  scheduleScan(WM_SCAN_SCHEDULE_USER_REFRESH, forceRefresh);
}

bool WiFiManager::shouldScheduleScan(unsigned int cachetime,
                                     wm_scan_schedule_reason_t reason,
                                     bool forceRefresh) const {
  if (forceRefresh) {
    return true;
  }

  switch (reason) {
    case WM_SCAN_SCHEDULE_USER_REFRESH:
      return true;

    case WM_SCAN_SCHEDULE_PRELOAD:
    case WM_SCAN_SCHEDULE_STALE_CACHE:
    case WM_SCAN_SCHEDULE_UI_RESUME:
      return !hasFreshScanResults(cachetime);

    case WM_SCAN_SCHEDULE_NONE:
    default:
      return false;
  }
}

void WiFiManager::scheduleScan(wm_scan_schedule_reason_t reason, bool forceRefresh) {
  if (_numNetworks == 0 && _autoforcerescan) {
    forceRefresh = true;
  }

  if (!shouldScheduleScan(_scancachetime, reason, forceRefresh)) {
    return;
  }

  _scan.schedulePending = true;
  _scan.forceRefresh = _scan.forceRefresh || forceRefresh;
  _scan.requestedAt = millis();
  _scan.scheduledReason = reason;

  if (_scan.state != WM_SCAN_RUNNING && _scan.state != WM_SCAN_QUEUED) {
    _scan.state = WM_SCAN_QUEUED;
    _scan.lastScanResult = WIFI_SCAN_RUNNING;
  }
}

void WiFiManager::processScan() {
  const unsigned long now = millis();

  if (_scan.completionPending) {
    const int completionResult = _scan.completionResult;
    const uint32_t completionGeneration = _scan.completionGeneration;
    _scan.completionPending = false;
    if (completionGeneration == _scan.runningGeneration) {
      if (completionResult >= 0) {
        finalizeAsyncScan(completionResult);
      } else {
        failAsyncScan(completionResult == WIFI_SCAN_FAILED ? WM_SCAN_FAILED : WM_SCAN_TIMEOUT, completionResult);
      }
    } else {
      _scan.completionGeneration = 0;
    }
  }

  if ((_scan.state == WM_SCAN_RUNNING || _scan.state == WM_SCAN_QUEUED) && !canRunAsyncScan()) {
    resetAsyncScan(false);
    return;
  }

  switch (_scan.state) {
    case WM_SCAN_IDLE:
    case WM_SCAN_COMPLETE:
    case WM_SCAN_FAILED:
    case WM_SCAN_TIMEOUT:
      if (_scan.schedulePending) {
        _scan.state = WM_SCAN_QUEUED;
      }
      break;

    case WM_SCAN_QUEUED:
      startAsyncScan();
      break;

    case WM_SCAN_RUNNING:
      if ((now - _scan.startedAt) > _scan.timeoutMs) {
        failAsyncScan(WM_SCAN_TIMEOUT);
        return;
      }

      #ifdef ESP32
        if (!_scan.completionPending) {
          int scanStatus = WiFi.scanComplete();
          if (scanStatus >= 0) {
            finalizeAsyncScan(scanStatus);
          } else if (scanStatus == WIFI_SCAN_FAILED) {
            failAsyncScan(WM_SCAN_FAILED, scanStatus);
          }
        }
      #endif
      break;
  }
}

bool WiFiManager::startAsyncScan() {
  const unsigned long now = millis();

  if (!canRunAsyncScan()) {
    return false;
  }

  if (_scan.finishedAt > 0 && (now - _scan.finishedAt) < _scan.minRestartIntervalMs) {
    return false;
  }

  if (_scan.forceRefresh) {
    invalidateScanResults();
  }

  _scan.completionPending = false;
  _scan.completionResult = WIFI_SCAN_FAILED;
  _scan.generation++;
  _scan.runningGeneration = _scan.generation;
  _scan.completionGeneration = 0;

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("WiFi Scan ASYNC started"));
  #endif

  #ifdef ESP8266
    #ifndef WM_NOASYNC
      using namespace std::placeholders;
      WiFi.scanDelete();
      WiFi.scanNetworksAsync(std::bind(&WiFiManager::WiFi_scanComplete, this, _1));
      _scan.state = WM_SCAN_RUNNING;
      _scan.startedAt = now;
      _scan.lastScanResult = WIFI_SCAN_RUNNING;
      _scan.schedulePending = false;
      _scan.forceRefresh = false;
      return true;
    #else
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] Async scan not available on this ESP8266 core"));
      #endif
      failAsyncScan(WM_SCAN_FAILED);
      return false;
    #endif
  #else
    WiFi.scanDelete();
    int scanResult = WiFi.scanNetworks(true);
    if (scanResult == WIFI_SCAN_RUNNING) {
      _scan.state = WM_SCAN_RUNNING;
      _scan.startedAt = now;
      _scan.lastScanResult = WIFI_SCAN_RUNNING;
      _scan.schedulePending = false;
      _scan.forceRefresh = false;
      return true;
    }

    if (scanResult >= 0) {
      finalizeAsyncScan(scanResult);
      return true;
    }

    failAsyncScan(WM_SCAN_FAILED, scanResult);
    return false;
  #endif
}

void WiFiManager::cacheScanResults(int networksFound) {
  _scanResultsCache.clear();
  if (networksFound <= 0) {
    return;
  }

  _scanResultsCache.reserve(static_cast<size_t>(networksFound));
  for (int i = 0; i < networksFound; i++) {
    WiFiScanNetwork network;
    network.ssid = WiFi.SSID(i);
    network.rssi = WiFi.RSSI(i);
    network.encType = WiFi.encryptionType(i);
    _scanResultsCache.push_back(network);
  }
}

void WiFiManager::finalizeAsyncScan(int networksFound) {
  if (_scan.completionGeneration != 0 && _scan.completionGeneration != _scan.runningGeneration) {
    return;
  }

  _lastscan = millis();
  _scan.finishedAt = _lastscan;
  _scan.lastScanResult = networksFound;
  _scan.state = WM_SCAN_COMPLETE;
  _scan.resultsValid = networksFound >= 0;
  _scan.schedulePending = false;
  _scan.forceRefresh = false;

  cacheScanResults(networksFound);
  _numNetworks = static_cast<int>(_scanResultsCache.size());
  _scan.visibleNetworkCount = _numNetworks;
  _scan.completionGeneration = 0;
  _scan.runningGeneration = 0;
  WiFi.scanDelete();

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("WiFi Scan ASYNC completed"), "in "+(String)(_scan.finishedAt - _scan.startedAt)+" ms");
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("WiFi Scan ASYNC found:"),_numNetworks);
  #endif
}

void WiFiManager::failAsyncScan(wm_scan_state_t state, int scanResult) {
  WiFi.scanDelete();
  _scan.finishedAt = millis();
  _scan.lastScanResult = scanResult;
  _scan.state = state;
  _scan.resultsValid = false;
  _scan.completionPending = false;
  _scan.schedulePending = false;
  _scan.forceRefresh = false;
  _scan.visibleNetworkCount = 0;
  _numNetworks = 0;
  _lastscan = 0;
  _scan.completionGeneration = 0;
  _scan.runningGeneration = 0;
  _scanResultsCache.clear();

  #ifndef WM_NO_LOG
  if (state == WM_SCAN_TIMEOUT) {
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] scan timed out"));
  } else {
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] scan failed"));
  }
  #endif
}

void WiFiManager::resetAsyncScan(bool clearResults) {
  WiFi.scanDelete();
  _scan.generation++;
  _scan.state = WM_SCAN_IDLE;
  _scan.resultsValid = clearResults ? false : _scan.resultsValid;
  _scan.schedulePending = false;
  _scan.forceRefresh = false;
  _scan.completionPending = false;
  _scan.requestedAt = 0;
  _scan.startedAt = 0;
  _scan.finishedAt = 0;
  _scan.lastScanResult = WIFI_SCAN_FAILED;
  _scan.completionResult = WIFI_SCAN_FAILED;
  _scan.runningGeneration = 0;
  _scan.completionGeneration = 0;
  _scan.scheduledReason = WM_SCAN_SCHEDULE_NONE;
  if (clearResults) {
    _numNetworks = 0;
    _lastscan = 0;
    _scan.visibleNetworkCount = 0;
    _scanResultsCache.clear();
  } else {
    _scan.visibleNetworkCount = _numNetworks;
  }
}

void WiFiManager::invalidateScanResults() {
  _scan.resultsValid = false;
  _scan.visibleNetworkCount = 0;
  _numNetworks = 0;
  _lastscan = 0;
  _scanResultsCache.clear();
}

bool WiFiManager::hasFreshScanResults(unsigned int cachetime) const {
  if (!_scan.resultsValid || _scanResultsCache.empty() || _lastscan == 0) {
    return false;
  }

  return cachetime == 0 || (millis() - _lastscan) <= cachetime;
}

bool WiFiManager::canRunAsyncScan() const {
  if (!configPortalActive && !webPortalActive) {
    return false;
  }

  if (_scanLifecycleBlocked) {
    return false;
  }

  if (isConfigPortalConnectPending() || _abortScheduled || _rebootScheduled || abort) {
    return false;
  }

  return true;
}

// PUBLIC

// METHODS

/**
 * reset wifi settings, clean stored ap password
 */

/**
 * [stopConfigPortal description]
 */
void WiFiManager::stopConfigPortal(){
  // Immediately shutdown the config portal
  if(configPortalActive) {
    abort = true; // Set abort flag for any in-flight async operations
    shutdownConfigPortal();
  }
}

/**
 * disconnect
 * @access public
 * @since $dev
 * @return bool success
 */
bool WiFiManager::disconnect(){
  if(WiFi.status() != WL_CONNECTED){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("Disconnecting: Not connected"));
    #endif
    return false;
  }  
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Disconnecting"));
  #endif
  return WiFi_Disconnect();
}

/**
 * reboot the device
 * @access public
 */
void WiFiManager::reboot(){
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Restarting"));
  #endif
  ESP.restart();
}

/**
 * reboot the device
 * @access public
 */
bool WiFiManager::erase(){
  return erase(false);
}

bool WiFiManager::erase(bool opt){
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, "Erasing");
  #endif

  #if defined(ESP32) && ((defined(WM_ERASE_NVS) || defined(nvs_flash_h)))
    // if opt true, do nvs erase
    if(opt){
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Erasing NVS"));
      #endif
      esp_err_t err;
      err = nvs_flash_init();
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("nvs_flash_init: "),err!=ESP_OK ? (String)err : "Success");
      #endif
      err = nvs_flash_erase();
      #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("nvs_flash_erase: "), err!=ESP_OK ? (String)err : "Success");
      #endif
      return err == ESP_OK;
    }
  #elif defined(ESP8266) && defined(spiffs_api_h)
    if(opt){
      bool ret = false;
      if(SPIFFS.begin()){
      #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Erasing SPIFFS"));
        #endif
        bool ret = SPIFFS.format();
        #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("spiffs erase: "),ret ? "Success" : "ERROR");
        #endif
      } else{
      #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[ERROR] Could not start SPIFFS"));
        #endif
      }
      return ret;
    }
  #else
    (void)opt;
  #endif

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("Erasing WiFi Config"));
  #endif
  return WiFi_eraseConfig();
}

/**
 * [resetSettings description]
 * ERASES STA CREDENTIALS
 * @access public
 */
void WiFiManager::resetSettings() {
#ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("resetSettings"));
  #endif
  WiFi_enableSTA(true,true); // must be sta to disconnect erase
  delay(500); // ensure sta is enabled
  if (_resetcallback != NULL){
      _resetcallback();  // @CALLBACK
  }
  
  #ifdef ESP32
    WiFi.disconnect(true,true);
  #else
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.persistent(false);
  #endif
  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("SETTINGS ERASED"));
  #endif
}

// SETTERS

/**
 * [setTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setTimeout(unsigned long seconds) {
  setConfigPortalTimeout(seconds);
}

/**
 * [setConfigPortalTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setConfigPortalTimeout(unsigned long seconds) {
  _configPortalTimeout = seconds * 1000;
}

/**
 * [setConnectTimeout description]
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setConnectTimeout(unsigned long seconds) {
  _connectTimeout = seconds * 1000;
}

/**
 * [setConnectRetries description]
 * @access public
 * @param {[type]} uint8_t numRetries [description]
 */
void WiFiManager::setConnectRetries(uint8_t numRetries){
  _connectRetries = constrain(numRetries,1,10);
}

/**
 * toggle _cleanconnect, always disconnect before connecting
 * @param {[type]} bool enable [description]
 */
void WiFiManager::setCleanConnect(bool enable){
  _cleanConnect = enable;
}

/**
 * [setConnectTimeout description
 * @access public
 * @param {[type]} unsigned long seconds [description]
 */
void WiFiManager::setSaveConnectTimeout(unsigned long seconds) {
  _saveTimeout = seconds * 1000;
}

/**
 * Set save portal connect on save option, 
 * if false, will only save credentials not connect
 * @access public
 * @param {[type]} bool connect [description]
 */
void WiFiManager::setSaveConnect(bool connect) {
  _connectonsave = connect;
}

void WiFiManager::setLogEnabled(boolean enabled) {
  _logEnabled = enabled;
  if (_logEnabled && (uint8_t)_runtimeMaxLevel == (uint8_t)WiFiManagerLogLevel::Trace) {
    debugPlatformInfo();
  }
  if (_logEnabled && (uint8_t)_runtimeMaxLevel >= (uint8_t)WiFiManagerLogLevel::Info) {
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, (__FlashStringHelper*)WM_VERSION_STR,
        String(F(" D:")) + String((uint8_t)_runtimeMaxLevel));
  }
}

void WiFiManager::setLogPrefix(String prefix) {
  _logPrefix = prefix;
}

void WiFiManager::setLogOutput(boolean enabled, WiFiManagerLogLevel maxLevel) {
  _runtimeMaxLevel = maxLevel;
  setLogEnabled(enabled);
}

void WiFiManager::setLogSink(WiFiManagerLogSink* sink) {
  _logSink = sink;
}

WiFiManagerLogSink* WiFiManager::getLogSink() {
  return _logSink;
}

void WiFiManager::log(WiFiManagerLogLevel level, const char* subsystem, const String& text) {
#ifndef WM_NO_LOG
  emitLogImpl(level, subsystem, text, String());
#endif
}

void WiFiManager::emitLogImpl(WiFiManagerLogLevel level, const char* subsystem, const String& textA,
                               const String& textB) {
#ifndef WM_NO_LOG
  if (!_logEnabled || (uint8_t)_runtimeMaxLevel < (uint8_t)level) {
    return;
  }

#if defined(ESP32)
  if ((uint8_t)_runtimeMaxLevel >= (uint8_t)WiFiManagerLogLevel::Trace) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    uint32_t free = info.total_free_bytes;
    uint16_t maxb = info.largest_free_block;
    uint8_t frag = (free > 0) ? static_cast<uint8_t>(100 - (maxb * 100) / free) : 0;
    String memLine;
    memLine.reserve(64);
    memLine += F("[MEM] free: ");
    memLine += String(free);
    memLine += F(" | max: ");
    memLine += String(maxb);
    memLine += F(" | frag: ");
    memLine += String(frag);
    memLine += F("%");
    WiFiManagerLogMessage memMsg{WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem, memLine};
    if (_logSink) {
      _logSink->log(memMsg);
    } else {
      _logPort.print(memLine);
      _logPort.println();
    }
  }
#endif

  String line;
  line.reserve(96u + textA.length() + textB.length());
  line += _logPrefix;
  if ((uint8_t)_runtimeMaxLevel >= (uint8_t)_logTagMinLevel) {
    line += F("[");
    line += String(static_cast<int>((uint8_t)level));
    line += F("] ");
  }
  const char* sub = subsystem ? subsystem : kWiFiMgrLogSubsystem;
  if (strcmp(sub, kWiFiMgrLogSubsystem) != 0) {
    line += F("[");
    line += sub;
    line += F("] ");
  }
  line += textA;
  if (textB.length() > 0) {
    line += F(" ");
    line += textB;
  }

  WiFiManagerLogMessage msg{level, sub, std::move(line)};
  if (_logSink) {
    _logSink->log(msg);
  } else {
    _logPort.print(msg.line);
    _logPort.println();
  }
#endif
}


/**
 * [setAPStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void WiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _ap_static_ip = ip;
  _ap_static_gw = gw;
  _ap_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 */
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _sta_static_ip = ip;
  _sta_static_gw = gw;
  _sta_static_sn = sn;
}

/**
 * [setSTAStaticIPConfig description]
 * @since $dev
 * @access public
 * @param {[type]} IPAddress ip [description]
 * @param {[type]} IPAddress gw [description]
 * @param {[type]} IPAddress sn [description]
 * @param {[type]} IPAddress dns [description]
 */
void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns) {
  setSTAStaticIPConfig(ip,gw,sn);
  _sta_static_dns = dns;
}

/**
 * [setMinimumSignalQuality description]
 * @access public
 * @param {[type]} int quality [description]
 */
void WiFiManager::setMinimumSignalQuality(int quality) {
  _minimumQuality = quality;
}

/**
 * [setBreakAfterConfig description]
 * @access public
 * @param {[type]} boolean shouldBreak [description]
 */
void WiFiManager::setBreakAfterConfig(boolean shouldBreak) {
  _shouldBreakAfterConfig = shouldBreak;
}

/**
 * setAPCallback, set a callback when softap is started
 * @access public 
 * @param {[type]} void (*func)(WiFiManager* wminstance)
 */
void WiFiManager::setAPCallback( std::function<void(WiFiManager*)> func ) {
  _apcallback = func;
}

/**
 * setWebServerCallback, set a callback after webserver is reset, and before routes are setup
 * if we set webserver handlers before wm, they are used and wm is not by esp webserver
 * on events cannot be overrided once set, and are not mutiples
 * @access public 
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setWebServerCallback( std::function<void()> func ) {
  _webservercallback = func;
}

/**
 * setSaveConfigCallback, set a save config callback after closing configportal
 * @note calls only if wifi is saved or changed, or setBreakAfterConfig(true)
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setSaveConfigCallback( std::function<void()> func ) {
  _savewificallback = func;
}

/**
 * setPreSaveConfigCallback, set a callback to fire before saving wifi or params
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreSaveConfigCallback( std::function<void()> func ) {
  _presavewificallback = func;
}

/**
 * setConfigResetCallback, set a callback to occur when a resetSettings() occurs
 * @access public
 * @param {[type]} void(*func)(void)
 */
void WiFiManager::setConfigResetCallback( std::function<void()> func ) {
    _resetcallback = func;
}

/**
 * setSaveParamsCallback, set a save params callback on params save in wifi or params pages
 * @access public
 * @param {[type]} void (*func)(WiFiManagerRequestArgs)
 */
void WiFiManager::setSaveParamsCallback( std::function<void(WiFiManagerRequestArgs)> func ) {
  _saveparamscallback = func;
}

/**
 * setPreSaveParamsCallback, set a pre save params callback on params save prior to anything else
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreSaveParamsCallback( std::function<void()> func ) {
  _presaveparamscallback = func;
}

/**
 * setPreOtaUpdateCallback, set a callback to fire before OTA update
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setPreOtaUpdateCallback( std::function<void()> func ) {
  _preotaupdatecallback = func;
}

/**
 * setConfigPortalTimeoutCallback, set a callback to config portal is timeout
 * @access public
 * @param {[type]} void (*func)(void)
 */
void WiFiManager::setConfigPortalTimeoutCallback( std::function<void()> func ) {
  _configportaltimeoutcallback = func;
}

/**
 * toggle wifiscan hiding of duplicate ssid names
 * if this is false, wifiscan will remove duplicat Access Points - defaut true
 * @access public
 * @param boolean removeDuplicates [true]
 */
void WiFiManager::setRemoveDuplicateAPs(boolean removeDuplicates) {
  _removeDuplicateAPs = removeDuplicates;
}

/**
 * toggle restore persistent, track internally
 * sets ESP wifi.persistent so we can remember it and restore user preference on destruct
 * there is no getter in esp8266 platform prior to https://github.com/esp8266/Arduino/pull/3857
 * @since $dev
 * @access public
 * @param boolean persistent [true]
 */
void WiFiManager::setRestorePersistent(boolean persistent) {
  _userpersistent = persistent;
  if(!persistent){
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("persistent is off"));
    #endif
  }
}

/**
 * toggle showing static ip form fields
 * if enabled, then the static ip, gateway, subnet fields will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void WiFiManager::setShowStaticFields(boolean alwaysShow){
  if(_disableIpFields) _staShowStaticFields = alwaysShow ? 1 : -1;
  else _staShowStaticFields = alwaysShow ? 1 : 0;
}

/**
 * toggle showing dns fields
 * if enabled, then the dns1 field will be visible, even if not set in code
 * @since $dev
 * @access public
 * @param boolean alwaysShow [false]
 */
void WiFiManager::setShowDnsFields(boolean alwaysShow){
  if(_disableIpFields) _staShowDns = alwaysShow ? 1 : -1;
  else _staShowDns = alwaysShow ? 1 : 0;
}

/**
 * toggle wifi autoreconnect policy
 * if enabled, then wifi will autoreconnect automatically always
 * On esp8266 we force this on when autoconnect is called, see notes
 * On esp32 this is handled on SYSTEM_EVENT_STA_DISCONNECTED since it does not exist in core yet
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setWiFiAutoReconnect(boolean enabled){
  _wifiAutoReconnect = enabled;
}

/**
 * toggle configportal timeout wait for station client
 * if enabled, then the configportal will start timeout when no stations are connected to softAP
 * disabled by default as rogue stations can keep it open if there is no auth
 * @since $dev
 * @access public
 * @param boolean enabled [false]
 */
void WiFiManager::setAPClientCheck(boolean enabled){
  _apClientCheck = enabled;
}

/**
 * toggle configportal timeout wait for web client
 * if enabled, then the configportal will restart timeout when client requests come in
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setWebPortalClientCheck(boolean enabled){
  _webClientCheck = enabled;
}

/**
 * toggle wifiscan percentages or quality icons
 * @since $dev
 * @access public
 * @param boolean enabled [false]
 */
void WiFiManager::setScanDispPerc(boolean enabled){
  _scanDispOptions = enabled;
}

/**
 * toggle configportal if autoconnect failed
 * if enabled, then the configportal will be activated on autoconnect failure
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setEnableConfigPortal(boolean enable)
{
    _enableConfigPortal = enable;
}

/**
 * toggle configportal if autoconnect failed
 * if enabled, then the configportal will be de-activated on wifi save
 * @since $dev
 * @access public
 * @param boolean enabled [true]
 */
void WiFiManager::setDisableConfigPortal(boolean enable)
{
    _disableConfigPortal = enable;
}

/**
 * set the hostname (dhcp client id)
 * @since $dev
 * @access public
 * @param  char* hostname 32 character hostname to use for sta+ap in esp32, sta in esp8266
 * @return bool false if hostname is not valid
 */
bool  WiFiManager::setHostname(const char * hostname){
  if (hostname == nullptr) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] hostname: null value rejected"));
    #endif
    return false;
  }

  String candidate(hostname);
  if (!normalizeHostname(candidate)) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] hostname: invalid value rejected"));
    #endif
    return false;
  }

  _hostname = candidate;
  return true;
}

bool  WiFiManager::setHostname(String hostname){
  if (!normalizeHostname(hostname)) {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem, F("[ERROR] hostname: invalid value rejected"));
    #endif
    return false;
  }

  _hostname = hostname;
  return true;
}

/**
 * set the soft ao channel, ignored if channelsync is true and connected
 * @param int32_t   wifi channel, 0 to disable
 */
void WiFiManager::setWiFiAPChannel(int32_t channel){
  _apChannel = channel;
}

/**
 * set the soft ap hidden
 * @param bool   wifi ap hidden, default is false
 */
void WiFiManager::setWiFiAPHidden(bool hidden){
  _apHidden = hidden;
}


void WiFiManager::portalSetBrandTitle(const String& title) {
  _portalBrand.title = title;
}

void WiFiManager::portalSetContextIdentityText(const String& identityText) {
  _portalBrand.identityTextOverride = identityText;
}

void WiFiManager::portalSetBrandHomeIntro(const String& text) {
  _portalBrand.homeIntro = text;
}

void WiFiManager::portalSetBrandLogoSvg(const String& svgMarkup) {
  _portalBrand.logoSvg = svgMarkup;
}

void WiFiManager::portalSetPageInfoVisible(bool visible) {
  _portalPages.infoVisible = visible;
}

void WiFiManager::portalSetPageUpdateVisible(bool visible) {
  _portalPages.updateVisible = visible;
}

void WiFiManager::portalSetPageSetupVisible(bool visible) {
  _portalPages.setupVisible = visible;
}

void WiFiManager::portalSetActionEraseVisible(bool visible) {
  _portalActions.eraseVisible = visible;
}

void WiFiManager::portalSetActionRestartVisible(bool visible) {
  _portalActions.restartVisible = visible;
}

void WiFiManager::portalSetActionExitVisible(bool visible) {
  _portalActions.exitVisible = visible;
}

void WiFiManager::portalSetActionCloseCaptiveVisible(bool visible) {
  _portalActions.closeCaptiveVisible = visible;
}

void WiFiManager::portalSetActionBackVisible(bool visible) {
  _portalActions.backVisible = visible;
}

void WiFiManager::portalSetLayoutParamsLocation(PortalParamsLocation location) {
  _portalLayout.paramsOnWifiPage = (location == PortalParamsLocation::WiFiPage);
}

void WiFiManager::portalSetBehaviorCaptivePortalEnabled(bool enabled) {
  _enableCaptivePortal = enabled;
}

void WiFiManager::portalSetBehaviorConnectOnSave(bool enabled) {
  setSaveConnect(enabled);
}

void WiFiManager::portalSetBehaviorExitAllowed(bool allowed) {
  _allowExit = allowed;
}

void WiFiManager::portalSetBehaviorConnectTimeoutSeconds(unsigned long seconds) {
  setConnectTimeout(seconds);
}

void WiFiManager::portalSetBehaviorPortalTimeoutSeconds(unsigned long seconds) {
  setConfigPortalTimeout(seconds);
}

void WiFiManager::portalSetBehaviorAutoReconnect(bool enabled) {
  setWiFiAutoReconnect(enabled);
}

void WiFiManager::portalSetBehaviorApClientCheck(bool enabled) {
  setAPClientCheck(enabled);
}

void WiFiManager::portalSetBehaviorWebClientCheck(bool enabled) {
  setWebPortalClientCheck(enabled);
}

void WiFiManager::portalSetFieldPasswordPlaceholderMode(PortalPasswordPlaceholderMode mode) {
  _portalPasswordPlaceholderMode = mode;
}

void WiFiManager::portalSetFieldStaticIpVisibility(PortalFieldVisibility visibility) {
  switch (visibility) {
    case PortalFieldVisibility::Always:
      setShowStaticFields(true);
      break;
    case PortalFieldVisibility::Auto:
      setShowStaticFields(false);
      break;
    case PortalFieldVisibility::Hidden:
      _staShowStaticFields = -1;
      break;
  }
}

void WiFiManager::portalSetFieldStaticDnsVisibility(PortalFieldVisibility visibility) {
  switch (visibility) {
    case PortalFieldVisibility::Always:
      setShowDnsFields(true);
      break;
    case PortalFieldVisibility::Auto:
      setShowDnsFields(false);
      break;
    case PortalFieldVisibility::Hidden:
      _staShowDns = -1;
      break;
  }
}

void WiFiManager::portalAddInfoSection(const PortalInfoSection& section) {
  _portalStructured.infoSections.push_back(section);
}

void WiFiManager::portalClearInfoSections() {
  _portalStructured.infoSections.clear();
}

void WiFiManager::portalAddHomeCard(const PortalHomeCard& card) {
  _portalStructured.homeCards.push_back(card);
}

void WiFiManager::portalClearHomeCards() {
  _portalStructured.homeCards.clear();
}

void WiFiManager::portalAppendCss(const String& css) {
  _portalAssets.appendedCss += css;
}

void WiFiManager::portalOverrideCss(const String& css) {
  _portalAssets.overriddenCss = css;
}

void WiFiManager::portalAppendJs(const String& js) {
  _portalAssets.appendedJs += js;
}

/**
 * check if the config portal is running
 * @return bool true if active
 */
bool WiFiManager::getConfigPortalActive() const {
  return configPortalActive;
}

/**
 * [getConfigPortalActive description]
 * @return bool true if active
 */
bool WiFiManager::getWebPortalActive(){
  return webPortalActive;
}

bool WiFiManager::hasEnteredConfigPortal() const {
  return _hasEnteredConfigPortal;
}

WiFiManager::wm_configportal_connect_state_t WiFiManager::getConfigPortalConnectState() const {
  switch (_cpConnectState) {
    case wm_cp_connect_state_t::queued:
    case wm_cp_connect_state_t::delaying:
    case wm_cp_connect_state_t::starting:
    case wm_cp_connect_state_t::waiting:
      return WM_CP_CONNECT_WAITING;
    case wm_cp_connect_state_t::success_waiting_close:
    case wm_cp_connect_state_t::success:
      return WM_CP_CONNECT_SUCCESS;
    case wm_cp_connect_state_t::failed:
      return WM_CP_CONNECT_FAILED;
    case wm_cp_connect_state_t::idle:
    default:
      return WM_CP_CONNECT_IDLE;
  }
}

bool WiFiManager::isConfigPortalConnectPending() const {
  return _cpConnectState == wm_cp_connect_state_t::queued || _cpConnectState == wm_cp_connect_state_t::delaying
         || _cpConnectState == wm_cp_connect_state_t::starting || _cpConnectState == wm_cp_connect_state_t::waiting
         || _cpConnectState == wm_cp_connect_state_t::success_waiting_close;
}

bool WiFiManager::didConfigPortalConnectSucceed() const {
  return _cpConnectState == wm_cp_connect_state_t::success
         || _cpConnectState == wm_cp_connect_state_t::success_waiting_close;
}

bool WiFiManager::didConfigPortalConnectFail() const {
  return _cpConnectState == wm_cp_connect_state_t::failed;
}

uint8_t WiFiManager::getConfigPortalConnectStatus() const {
  return _cpConnectStatus;
}

String WiFiManager::getConfigPortalConnectMessage() const {
  return _cpConnectMessage;
}

void WiFiManager::setEventCallback(WiFiManagerEventCallback cb) {
  _eventCallback = cb;
}

String WiFiManager::getWiFiHostname(){
  #ifdef ESP32
    return (String)WiFi.getHostname();
  #else
    return (String)WiFi.hostname();
  #endif
}

AsyncWebServer* WiFiManager::getServer() {
  return _serverManager ? _serverManager->getServer() : nullptr;
}

DNSServer* WiFiManager::getDNSServer() {
  return _serverManager ? _serverManager->getDNSServer() : nullptr;
}

// GETTERS

/**
 * get config portal AP SSID
 * @since 0.0.1
 * @access public
 * @return String the configportal ap name
 */
String WiFiManager::getConfigPortalSSID() {
  return _apName;
}

/**
 * return the last known connection result
 * logged on autoconnect and portal credential-save attempts; can be used to check why failed
 * get as readable string with getWLStatusString(getLastConxResult);
 * @since $dev
 * @access public
 * @return bool return wl_status codes
 */
uint8_t WiFiManager::getLastConxResult(){
  return _lastconxresult;
}

/**
 * check if wifi has a saved ap or not
 * @since $dev
 * @access public
 * @return bool true if a saved ap config exists
 */
bool WiFiManager::getWiFiIsSaved(){
  return WiFi_hasAutoConnect();
}

/**
 * getDefaultAPName
 * @since $dev
 * @return string 
 */
String WiFiManager::getDefaultAPName(){
  String hostString = String(WIFI_getChipId(),HEX);
  hostString.toUpperCase();
  // char hostString[16] = {0};
  // sprintf(hostString, "%06X", ESP.getChipId());  
  return _wifissidprefix + "_" + hostString;
}

/**
 * setWiFiSSIDPrefix
 * Set the prefix for default AP name (e.g., "ESP" becomes "ESP_XXXXXX")
 * @param String prefix, prefix for auto-generated AP name
 */
void WiFiManager::setWiFiSSIDPrefix(String prefix){
  _wifissidprefix = prefix;
}

/**
 * setCountry
 * @since $dev
 * @param String cc country code, must be defined in WiFiSetCountry, US, JP, CN
 */
void WiFiManager::setCountry(String cc){
  _wificountry = cc;
}

/**
 * setHttpPort
 * @param uint16_t port webserver port number default 80
 */
void WiFiManager::setHttpPort(uint16_t port){
  _httpPort = port;
}


bool WiFiManager::preloadWiFi(String ssid, String pass){
  _defaultssid = ssid;
  _defaultpass = pass;
  return true;
}

// HELPERS

/**
 * getWiFiSSID
 * @since $dev
 * @param bool persistent
 * @return String
 */
String WiFiManager::getWiFiSSID(bool persistent){
  return WiFi_SSID(persistent);
}

/**
 * getWiFiPass
 * @since $dev
 * @param bool persistent
 * @return String
 */
String WiFiManager::getWiFiPass(bool persistent){
  return WiFi_psk(persistent);
} 

void WiFiManager::debugSoftAPConfig(){
    
    #ifdef ESP8266
      softap_config config;
      wifi_softap_get_config(&config);
      #if !defined(WM_NOCOUNTRY)
        wifi_country_t country;
        wifi_get_country(&country);
      #endif
    #elif defined(ESP32)
      wifi_country_t country;
      wifi_config_t conf_config;
      esp_wifi_get_config(WIFI_IF_AP, &conf_config); // == ESP_OK
      wifi_ap_config_t config = conf_config.ap;
      esp_wifi_get_country(&country);
    #endif

    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("SoftAP Configuration"));
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("--------------------"));
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("ssid:            "),(char *) config.ssid);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("password:        "),(char *) config.password);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("ssid_len:        "),config.ssid_len);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("channel:         "),config.channel);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("authmode:        "),config.authmode);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("ssid_hidden:     "),config.ssid_hidden);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("max_connection:  "),config.max_connection);
    #endif
    #if !defined(WM_NOCOUNTRY) 
    #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("country:         "),(String)country.cc);
      #endif
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("beacon_interval: "),(String)config.beacon_interval + "(ms)");
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("--------------------"));
    #endif
}

/**
 * [debugPlatformInfo description]
 * @access public
 * @return {[type]} [description]
 */
void WiFiManager::debugPlatformInfo(){
  #ifdef ESP8266
    system_print_meminfo();
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] getCoreVersion():         "),ESP.getCoreVersion());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] system_get_sdk_version(): "),system_get_sdk_version());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] system_get_boot_version():"),system_get_boot_version());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] getFreeHeap():            "),(String)ESP.getFreeHeap());
    #endif
  #elif defined(ESP32)
  #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] WM version: "),
        String((__FlashStringHelper*)WM_VERSION_STR) + String(F(" D:")) + String((uint8_t)_runtimeMaxLevel));
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] Arduino version: "), VER_ARDUINO_STR);
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] ESP SDK version: "), ESP.getSdkVersion());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] Free heap:       "), ESP.getFreeHeap());
    #endif

    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] Chip ID:"),WIFI_getChipId());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] Chip Model:"), ESP.getChipModel());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] Chip Cores:"), ESP.getChipCores());
    log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("[SYS] Chip Rev:"),   ESP.getChipRevision());
    #endif
  #endif
}

// Utility function wrappers - delegate to WiFiManagerUtils namespace

int WiFiManager::getRSSIasQuality(int RSSI) {
  return WiFiManagerUtils::rssiToQuality(RSSI);
}

boolean WiFiManager::isIp(String str) {
  return WiFiManagerUtils::isValidIP(str);
}

String WiFiManager::toStringIp(IPAddress ip) {
  return WiFiManagerUtils::ipToString(ip);
}

boolean WiFiManager::validApPassword(){
  // check that ap password is valid, return false
  if (_apPassword == NULL) _apPassword = "";
  if (_apPassword != "") {
    if (!WiFiManagerUtils::isValidAPPassword(_apPassword)) {
    #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Info, kWiFiMgrLogSubsystem, F("AccessPoint set password is INVALID or <8 chars"));
      #endif
      _apPassword = "";
      return false; // Fail secure - invalid password
    }
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("AccessPoint set password is VALID"));
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,"ap pass",_apPassword);
    #endif
  }
  return true;
}

String WiFiManager::htmlEntities(String str, bool whitespace) {
  return WiFiManagerUtils::htmlEntities(str, whitespace);
}

String WiFiManager::getWLStatusString(uint8_t status){
  return WiFiManagerUtils::getStatusString(status);
}

String WiFiManager::getWLStatusString(){
  return WiFiManagerUtils::getStatusString(WiFi.status());
}

String WiFiManager::encryptionTypeStr(uint8_t authmode) {
  return WiFiManagerUtils::getEncryptionString(authmode);
}

String WiFiManager::getModeString(uint8_t mode){
  return WiFiManagerUtils::getModeString(mode);
}

// Country configurations (used by WiFiSetCountry)
#ifdef ESP32
const wifi_country_t WM_COUNTRY_US{"US",1,11,CONFIG_ESP32_PHY_MAX_WIFI_TX_POWER,WIFI_COUNTRY_POLICY_AUTO};
const wifi_country_t WM_COUNTRY_CN{"CN",1,13,CONFIG_ESP32_PHY_MAX_WIFI_TX_POWER,WIFI_COUNTRY_POLICY_AUTO};
const wifi_country_t WM_COUNTRY_JP{"JP",1,14,CONFIG_ESP32_PHY_MAX_WIFI_TX_POWER,WIFI_COUNTRY_POLICY_AUTO};
#elif defined(ESP8266) && !defined(WM_NOCOUNTRY)
const wifi_country_t WM_COUNTRY_US{"US",1,11,WIFI_COUNTRY_POLICY_AUTO};
const wifi_country_t WM_COUNTRY_CN{"CN",1,13,WIFI_COUNTRY_POLICY_AUTO};
const wifi_country_t WM_COUNTRY_JP{"JP",1,14,WIFI_COUNTRY_POLICY_AUTO};
#endif

bool WiFiManager::WiFiSetCountry(){
  if(_wificountry == "") return false; // skip not set

  #ifndef WM_NO_LOG
  log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("WiFiSetCountry to"),_wificountry);
  #endif

  bool ret = true;
  #ifdef ESP32
  esp_err_t err = ESP_OK;
  if(WiFi.getMode() == WIFI_MODE_NULL){
      log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,"[ERROR] cannot set country, wifi not init");        
      // TODO: Document precondition: WiFi must be initialized (mode != NULL) before setting country
      return false;
  }
  // Assumes that _wificountry is set to one of the supported country codes : "01"(world safe mode) "AT","AU","BE","BG","BR",
  //               "CA","CH","CN","CY","CZ","DE","DK","EE","ES","FI","FR","GB","GR","HK","HR","HU",
  //               "IE","IN","IS","IT","JP","KR","LI","LT","LU","LV","MT","MX","NL","NO","NZ","PL","PT",
  //               "RO","SE","SI","SK","TW","US"
  // If an invalid country code is passed, ESP_ERR_WIFI_ARG will be returned
  // This also uses 802.11d mode, which matches the STA to the country code of the AP it connects to (meaning
  // that the country code will be overridden if connecting to a "foreign" AP)
  else {
    #ifndef WM_NOCOUNTRY
    err = esp_wifi_set_country_code(_wificountry.c_str(), true);
    #else
    log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,"[ERROR] esp wifi set country is not available");
    err = true;
    #endif
  }
  #ifndef WM_NO_LOG
    if(err){
      if(err == ESP_ERR_WIFI_NOT_INIT) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,"[ERROR] ESP_ERR_WIFI_NOT_INIT");
      else if(err == ESP_ERR_INVALID_ARG) log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,"[ERROR] ESP_ERR_WIFI_ARG (invalid country code)");
      else if(err != ESP_OK)log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,"[ERROR] unknown error",(String)err);
    }
  #endif
  ret = err == ESP_OK;
  
  #elif defined(ESP8266) && !defined(WM_NOCOUNTRY)
       // if(WiFi.getMode() == WIFI_OFF); // exception if wifi not init!
       if(_wificountry == "US") ret = wifi_set_country((wifi_country_t*)&WM_COUNTRY_US);
  else if(_wificountry == "JP") ret = wifi_set_country((wifi_country_t*)&WM_COUNTRY_JP);
  else if(_wificountry == "CN") ret = wifi_set_country((wifi_country_t*)&WM_COUNTRY_CN);
  #ifndef WM_NO_LOG
  else log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] country code not found"));
  #endif
  #endif
  
  #ifndef WM_NO_LOG
  if(ret) log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("[OK] esp_wifi_set_country: "),_wificountry);
  else log(WiFiManagerLogLevel::Error, kWiFiMgrLogSubsystem,F("[ERROR] esp_wifi_set_country failed"));  
  #endif
  return ret;
}

// set mode ignores WiFi.persistent 
bool WiFiManager::WiFi_Mode(WiFiMode_t m,bool persistent) {
    bool ret;
    #ifdef ESP8266
      if((wifi_get_opmode() == (uint8) m ) && !persistent) {
          return true;
      }
      ETS_UART_INTR_DISABLE();
      if(persistent) ret = wifi_set_opmode(m);
      else ret = wifi_set_opmode_current(m);
      ETS_UART_INTR_ENABLE();
    return ret;
    #elif defined(ESP32)
      if(persistent && esp32persistent) WiFi.persistent(true);
      ret = WiFi.mode(m);
      if(persistent && esp32persistent) WiFi.persistent(false);
      return ret;
    #endif
}
bool WiFiManager::WiFi_Mode(WiFiMode_t m) {
	return WiFi_Mode(m,false);
}

// sta disconnect without persistent
bool WiFiManager::WiFi_Disconnect() {
    #ifdef ESP8266
      if((WiFi.getMode() & WIFI_STA) != 0) {
          bool ret;
          #ifndef WM_NO_LOG
          log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("WiFi station disconnect"));
          #endif
          ETS_UART_INTR_DISABLE(); // @todo possibly not needed
          ret = wifi_station_disconnect();
          ETS_UART_INTR_ENABLE();        
          return ret;
      }
    #elif defined(ESP32)
    #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("WiFi station disconnect"));
      #endif
      return WiFi.disconnect(); // not persistent atm
    #endif
    return false;
}

// toggle STA without persistent
bool WiFiManager::WiFi_enableSTA(bool enable,bool persistent) {
#ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("WiFi_enableSTA"),(String) enable? "enable" : "disable");
    #endif
    #ifdef ESP8266
      WiFiMode_t newMode;
      WiFiMode_t currentMode = WiFi.getMode();
      bool isEnabled         = (currentMode & WIFI_STA) != 0;
      if(enable) newMode     = (WiFiMode_t)(currentMode | WIFI_STA);
      else newMode           = (WiFiMode_t)(currentMode & (~WIFI_STA));

      if((isEnabled != enable) || persistent) {
          if(enable) {
          #ifndef WM_NO_LOG
          	if(persistent) log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("enableSTA PERSISTENT ON"));
            #endif
              return WiFi_Mode(newMode,persistent);
          }
          else {
              return WiFi_Mode(newMode,persistent);
          }
      } else {
          return true;
      }
    #elif defined(ESP32)
      bool ret;
      if(persistent && esp32persistent) WiFi.persistent(true);
      ret = WiFi.enableSTA(enable);
      if(persistent && esp32persistent) WiFi.persistent(false);
      return ret;
    #endif
}

bool WiFiManager::WiFi_enableSTA(bool enable) {
	return WiFi_enableSTA(enable,false);
}

bool WiFiManager::WiFi_eraseConfig() {
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Trace, kWiFiMgrLogSubsystem,F("WiFi_eraseConfig"));
    #endif

    #ifdef ESP8266
      #ifndef WM_FIXERASECONFIG 
        return ESP.eraseConfig();
      #else
        // erase config BUG replacement
        // https://github.com/esp8266/Arduino/pull/3635
        const size_t cfgSize = 0x4000;
        size_t cfgAddr = ESP.getFlashChipSize() - cfgSize;

        for (size_t offset = 0; offset < cfgSize; offset += SPI_FLASH_SEC_SIZE) {
            if (!ESP.flashEraseSector((cfgAddr + offset) / SPI_FLASH_SEC_SIZE)) {
                return false;
            }
        }
        return true;
      #endif
    #elif defined(ESP32)

      bool ret;
      WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
      WiFi.persistent(true);
      ret = WiFi.disconnect(true,true); // disconnect(bool wifioff, bool eraseap)
      delay(500);
      WiFi.persistent(false);
      return ret;
    #endif
}

uint8_t WiFiManager::WiFi_softap_num_stations(){
  #ifdef ESP8266
    return wifi_softap_get_station_num();
  #elif defined(ESP32)
    return WiFi.softAPgetStationNum();
  #endif
}

bool WiFiManager::WiFi_hasAutoConnect(){
  return WiFi_SSID(true) != "";
}

String WiFiManager::WiFi_SSID(bool persistent) const{

    #ifdef ESP8266
    struct station_config conf;
    if(persistent) wifi_station_get_config_default(&conf);
    else wifi_station_get_config(&conf);

    char tmp[33]; //ssid can be up to 32chars, => plus null term
    memcpy(tmp, conf.ssid, sizeof(conf.ssid));
    tmp[32] = 0; //nullterm in case of 32 char ssid
    return String(reinterpret_cast<char*>(tmp));
    
    #elif defined(ESP32)
    if(persistent){
      wifi_config_t conf;
      esp_wifi_get_config(WIFI_IF_STA, &conf);
      return String(reinterpret_cast<const char*>(conf.sta.ssid));
    }
    else {
      if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
          return String();
      }
      wifi_ap_record_t info;
      if(!esp_wifi_sta_get_ap_info(&info)) {
          return String(reinterpret_cast<char*>(info.ssid));
      }
      return String();
    }
    #endif
}

String WiFiManager::WiFi_psk(bool persistent) const {
    #ifdef ESP8266
    struct station_config conf;

    if(persistent) wifi_station_get_config_default(&conf);
    else wifi_station_get_config(&conf);

    char tmp[65]; //psk is 64 bytes hex => plus null term
    memcpy(tmp, conf.password, sizeof(conf.password));
    tmp[64] = 0; //null term in case of 64 byte psk
    return String(reinterpret_cast<char*>(tmp));
    
    #elif defined(ESP32)
    // only if wifi is init
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
      return String();
    }
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return String(reinterpret_cast<char*>(conf.sta.password));
    #endif
}

#ifdef ESP32
  #ifdef WM_ARDUINOEVENTS
  void WiFiManager::WiFiEvent(WiFiEvent_t event,arduino_event_info_t info){
  #else
  void WiFiManager::WiFiEvent(WiFiEvent_t event,system_event_info_t info){
    #define wifi_sta_disconnected disconnected
    #define ARDUINO_EVENT_WIFI_STA_DISCONNECTED SYSTEM_EVENT_STA_DISCONNECTED
    #define ARDUINO_EVENT_WIFI_SCAN_DONE SYSTEM_EVENT_SCAN_DONE
  #endif
    if(!_hasBegun){
      return;
    }
    if(event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED){
    #ifndef WM_NO_LOG
      log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("[EVENT] WIFI_REASON: "),info.wifi_sta_disconnected.reason);
      #endif
      if(info.wifi_sta_disconnected.reason == WIFI_REASON_AUTH_EXPIRE || info.wifi_sta_disconnected.reason == WIFI_REASON_AUTH_FAIL){
        _lastconxresulttmp = 7; // Wrong password detected - SDK emits WIFI_REASON_AUTH_EXPIRE on some routers on auth_fail
      } else _lastconxresulttmp = WiFi.status();
      #ifndef WM_NO_LOG
      if(info.wifi_sta_disconnected.reason == WIFI_REASON_NO_AP_FOUND) log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("[EVENT] WIFI_REASON: NO_AP_FOUND"));
      if(info.wifi_sta_disconnected.reason == WIFI_REASON_ASSOC_FAIL){
        if(_aggresiveReconn && _connectRetries<4) _connectRetries=4;
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("[EVENT] WIFI_REASON: AUTH FAIL"));
      }  
      #endif
      #ifdef esp32autoreconnect
      #ifndef WM_NO_LOG
        log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("[Event] SYSTEM_EVENT_STA_DISCONNECTED, reconnecting"));
        #endif
        WiFi.reconnect();
      #endif
  }
  else if(event == ARDUINO_EVENT_WIFI_SCAN_DONE){
    uint16_t scans = WiFi.scanComplete();
    WiFi_scanComplete(scans);
  }
}
#endif

void WiFiManager::WiFi_autoReconnect(){
  #ifdef ESP8266
    WiFi.setAutoReconnect(_wifiAutoReconnect);
  #elif defined(ESP32)
    // Enable ESP32 event handler for auto-reconnect
    #ifndef WM_NO_LOG
    log(WiFiManagerLogLevel::Debug, kWiFiMgrLogSubsystem,F("ESP32 event handler enabled"));
    #endif
    using namespace std::placeholders;
    if(wm_event_id == 0) wm_event_id = WiFi.onEvent(std::bind(&WiFiManager::WiFiEvent,this,_1,_2));
  #endif
}

#ifdef UNIT_TEST
void WiFiManager::wmTestSetConnectPending(bool active) {
  _cpConnectState = active ? wm_cp_connect_state_t::waiting : wm_cp_connect_state_t::idle;
}
#endif

#endif
