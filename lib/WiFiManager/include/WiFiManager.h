/**
 * WiFiManager.h
 *
 * WiFiManager, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 *
 * @author Creator tzapu
 * @author tablatronix
 * @author alexhopeoconnor
 * @license MIT
 */


#ifndef WiFiManager_h
#define WiFiManager_h

#if defined(ESP8266) || defined(ESP32)

#ifdef ESP8266
#include <core_version.h>
#endif

#include <vector>
#include <functional>
#include <string.h>

#include "WiFiManagerParameter.h"
#include "WiFiManagerLogLevel.h"

#ifdef WM_NODEBUG
#ifndef WM_NO_LOG
#define WM_NO_LOG
#endif
#endif

#ifndef WM_LOG_LEVEL
#define WM_LOG_LEVEL 0
#endif

// #define WM_DFTE_LOGGING    // opt-in: bridge DFTE logging into WiFiManager (see README)
// #define WM_MDNS            // includes MDNS, also set MDNS with sethostname
// #define WM_FIXERASECONFIG  // use erase flash fix
// #define WM_ERASE_NVS       // esp32 erase(true) will erase NVS
// #define WM_RTC             // esp32 info page will include reset reasons

// #define WIFI_MANAGER_OVERRIDE_STRINGS // build flag for using own strings include

#ifdef ARDUINO_ESP8266_RELEASE_2_3_0
#warning "ARDUINO_ESP8266_RELEASE_2_3_0, some WM features disabled"
// @todo check failing on platform = espressif8266@1.7.3
#define WM_NOASYNC         // esp8266 no async scan wifi
#define WM_NOCOUNTRY       // esp8266 no country
#define WM_NOAUTH          // no httpauth
#define WM_NOSOFTAPSSID    // no softapssid() @todo shim
#endif

// #ifdef CONFIG_IDF_TARGET_ESP32S2
// #warning ESP32S2
// #endif

// #ifdef CONFIG_IDF_TARGET_ESP32C3
// #warning ESP32C3
// #endif

// #ifdef CONFIG_IDF_TARGET_ESP32S3
// #warning ESP32S3
// #endif

// #if defined(ARDUINO_ESP32S3_DEV) || defined(CONFIG_IDF_TARGET_ESP32S3)
// #warning "WM_NOTEMP"
// #define WM_NOTEMP // disabled temp sensor, have to determine which chip we are on
// #endif

// #include "soc/efuse_reg.h" // include to add efuse chip rev to info, getChipRevision() is almost always the same though, so not sure why it matters.

// #define esp32autoreconnect    // implement esp32 autoreconnect event listener kludge, @DEPRECATED
// autoreconnect is WORKING https://github.com/espressif/arduino-esp32/issues/653#issuecomment-405604766

// WM_WEBSERVERSHIM no longer needed - using ESPAsyncWebServer

#define WM_G(string_literal)  (String(FPSTR(string_literal)).c_str())

#ifdef ESP8266

    extern "C" {
      #include "user_interface.h"
    }
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>

    #ifdef WM_MDNS
        #include <ESP8266mDNS.h>
    #endif

    #define WIFI_getChipId() ESP.getChipId()
    #define WM_WIFIOPEN   ENC_TYPE_NONE

#elif defined(ESP32)

    #include <WiFi.h>
    #include <esp_wifi.h>
    #include <Update.h>
    #include <AsyncTCP.h>

    #define WIFI_getChipId() (uint32_t)ESP.getEfuseMac()
    #define WM_WIFIOPEN   WIFI_AUTH_OPEN

    #ifdef WM_ERASE_NVS
       #include <nvs.h>
       #include <nvs_flash.h>
    #endif

    #ifdef WM_MDNS
        #include <ESPmDNS.h>
    #endif

    #ifdef WM_RTC
        #ifdef ESP_IDF_VERSION_MAJOR // IDF 4+
        #if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
        #include "esp32/rom/rtc.h"
        #elif CONFIG_IDF_TARGET_ESP32S2
        #include "esp32s2/rom/rtc.h"
        #elif CONFIG_IDF_TARGET_ESP32C3
        #include "esp32c3/rom/rtc.h"
        #elif CONFIG_IDF_TARGET_ESP32S3
        #include "esp32s3/rom/rtc.h"
        #else
        #error Target CONFIG_IDF_TARGET is not supported
        #endif
        #else // ESP32 Before IDF 4.0
        #include "rom/rtc.h"
        #endif
    #endif

#else
#endif

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <memory>
#include <unordered_map>
#include <string>
// Include utility functions
#include "WiFiManagerUtils.h"
#include "WiFiManagerPortalUI.h"

// A station profile is intentionally fixed-size. ESP Wi-Fi accepts one station
// configuration at a time, so multi-network behaviour belongs to the
// application/controller rather than the SDK's saved station configuration.
constexpr uint8_t WM_STATION_PROFILE_COUNT = 2;
constexpr uint8_t WM_NO_STATION_PROFILE = 0xFF;

struct WiFiManagerStationProfile {
  bool enabled = false;
  bool hasPassword = false;
  char ssid[33] = {};
  char password[65] = {};
};

struct WiFiManagerStationProfiles {
  WiFiManagerStationProfile slots[WM_STATION_PROFILE_COUNT] = {};
  uint8_t preferredSlot = 0;
  uint8_t lastSuccessfulSlot = WM_NO_STATION_PROFILE;
};

/**
 * Optional durable backing for multi-profile station credentials. The manager
 * owns profile policy and never owns this store. A consumer that needs durable
 * profiles supplies one; otherwise the profiles remain in RAM.
 */
class WiFiManagerStationProfileStore {
public:
  virtual bool load(WiFiManagerStationProfiles& profiles) = 0;
  virtual bool save(const WiFiManagerStationProfiles& candidate) = 0;
  virtual bool clear() = 0;
  virtual ~WiFiManagerStationProfileStore() = default;
};

// prep string concat vars
#define WM_STRING2(x) #x
#define WM_STRING(x) WM_STRING2(x)

// WiFiManager version
const char WM_VERSION_STR[] PROGMEM = "v3.1.0";

// #include <esp_idf_version.h>
#ifdef ESP_IDF_VERSION
    // #pragma message "ESP_IDF_VERSION_MAJOR = " WM_STRING(ESP_IDF_VERSION_MAJOR)
    // #pragma message "ESP_IDF_VERSION_MINOR = " WM_STRING(ESP_IDF_VERSION_MINOR)
    // #pragma message "ESP_IDF_VERSION_PATCH = " WM_STRING(ESP_IDF_VERSION_PATCH)
    #define VER_IDF_STR WM_STRING(ESP_IDF_VERSION_MAJOR)  "."  WM_STRING(ESP_IDF_VERSION_MINOR)  "."  WM_STRING(ESP_IDF_VERSION_PATCH)
#else
    #define VER_IDF_STR "Unknown"
#endif

#ifdef Arduino_h
    #ifdef ESP32
    // #include "esp_arduino_version.h" // esp32 arduino > 2.x
    #endif
    // esp_get_idf_version
    #ifdef ESP_ARDUINO_VERSION
        // #pragma message "ESP_ARDUINO_VERSION_MAJOR = " WM_STRING(ESP_ARDUINO_VERSION_MAJOR)
        // #pragma message "ESP_ARDUINO_VERSION_MINOR = " WM_STRING(ESP_ARDUINO_VERSION_MINOR)
        // #pragma message "ESP_ARDUINO_VERSION_PATCH = " WM_STRING(ESP_ARDUINO_VERSION_PATCH)
        #ifdef ESP_ARDUINO_VERSION_MAJOR
        #define VER_ARDUINO_STR WM_STRING(ESP_ARDUINO_VERSION_MAJOR)  "."  WM_STRING(ESP_ARDUINO_VERSION_MINOR)  "."  WM_STRING(ESP_ARDUINO_VERSION_PATCH)
        #else
        #define VER_ARDUINO_STR "Unknown"
        #endif
    #else
        #include <core_version.h>
        // #pragma message "ESP_ARDUINO_VERSION_GIT  = " WM_STRING(ARDUINO_ESP32_GIT_VER)//  0x46d5afb1
        // #pragma message "ESP_ARDUINO_VERSION_DESC = " WM_STRING(ARDUINO_ESP32_GIT_DESC) //  1.0.6
        // #pragma message "ESP_ARDUINO_VERSION_REL  = " WM_STRING(ARDUINO_ESP32_RELEASE) //"1_0_6"
        #ifdef ESP_ARDUINO_VERSION_MAJOR
        #define VER_ARDUINO_STR WM_STRING(ESP_ARDUINO_VERSION_MAJOR)  "."  WM_STRING(ESP_ARDUINO_VERSION_MINOR)  "."  WM_STRING(ESP_ARDUINO_VERSION_PATCH)
        #else
        #define VER_ARDUINO_STR "Unknown"
        #endif
    #endif
#else
#define VER_ARDUINO_STR "Unknown"
#endif

// #pragma message "VER_IDF_STR = " WM_STRING(VER_IDF_STR)
// #pragma message "VER_ARDUINO_STR = " WM_STRING(VER_ARDUINO_STR)

// WiFiManagerRequestArgs is defined as a nested class inside WiFiManager (after WM_WebServer is defined)


// Forward declarations
class WiFiManagerServer;
class WiFiManagerHandlers;
class WiFiManagerDfteLogger;
class WiFiManagerLogSink;

// ---------------------------------------------------------------------------
// Portal customization (v2 contract: nested JSON; all entry points use portal* API)
// ---------------------------------------------------------------------------
enum class PortalParamsLocation : uint8_t { WiFiPage = 0, SetupPage = 1 };

enum class PortalPasswordPlaceholderMode : uint8_t { Hidden = 0, Masked = 1, Actual = 2 };

enum class PortalFieldVisibility : uint8_t { Hidden = 0, Auto = 1, Always = 2 };

enum class PortalHomeCardKind : uint8_t { Text = 0, Callout = 1, KeyValue = 2 };

struct PortalKeyValueItem {
  String key;
  String label;
  String value;
};

struct PortalInfoSection {
  String id;
  String title;
  std::vector<PortalKeyValueItem> items;
};

struct PortalHomeCard {
  String id;
  String title;
  PortalHomeCardKind kind = PortalHomeCardKind::KeyValue;
  String text;
  std::vector<PortalKeyValueItem> items;
};

struct PortalBrandState {
  WiFiManagerPortalText title;
  WiFiManagerPortalText identityTextOverride;
  WiFiManagerPortalText tagline;
  WiFiManagerPortalAsset logo;
  WiFiManagerPortalText logoAltText;
};

struct PortalPageState {
  bool infoVisible = true;
  bool updateVisible = true;
  bool setupVisible = true;
};

struct PortalActionState {
  bool eraseVisible = true;
  bool restartVisible = true;
  bool exitVisible = true;
  bool closeCaptiveVisible = true;
  bool backVisible = false;
};

struct PortalLayoutState {
  /** When true, custom parameters render on the Wi-Fi page; when false, only on #/setup. */
  bool paramsOnWifiPage = true;
};

struct PortalStructuredExtrasState {
  std::vector<PortalInfoSection> infoSections;
  std::vector<PortalHomeCard> homeCards;
};

class WiFiManager
{
  public:

    // Forward declare nested class - defined later after WM_WebServer is available
    class WiFiManagerRequestArgs;

    enum wm_scan_state_t : uint8_t {
      WM_SCAN_IDLE = 0,
      WM_SCAN_QUEUED,
      WM_SCAN_RUNNING,
      WM_SCAN_COMPLETE,
      WM_SCAN_FAILED,
      WM_SCAN_TIMEOUT,
    };

    enum wm_scan_schedule_reason_t : uint8_t {
      WM_SCAN_SCHEDULE_NONE = 0,
      WM_SCAN_SCHEDULE_PRELOAD,
      WM_SCAN_SCHEDULE_USER_REFRESH,
      WM_SCAN_SCHEDULE_STALE_CACHE,
      WM_SCAN_SCHEDULE_UI_RESUME,
    };

    struct WiFiScanNetwork {
      String ssid;
      int32_t rssi = 0;
      uint8_t encType = 0;
    };

    struct WiFiScanRuntimeState {
      wm_scan_state_t state = WM_SCAN_IDLE;
      bool resultsValid = false;
      bool schedulePending = false;
      bool forceRefresh = false;
      bool completionPending = false;
      unsigned long requestedAt = 0;
      unsigned long startedAt = 0;
      unsigned long finishedAt = 0;
      unsigned long timeoutMs = 15000;
      unsigned long minRestartIntervalMs = 2000;
      uint32_t generation = 0;
      uint32_t runningGeneration = 0;
      uint32_t completionGeneration = 0;
      int lastScanResult = WIFI_SCAN_FAILED;
      int completionResult = WIFI_SCAN_FAILED;
      int visibleNetworkCount = 0;
      wm_scan_schedule_reason_t scheduledReason = WM_SCAN_SCHEDULE_NONE;
    };

    /** Exposed to consumers: high-level result of a portal "save & connect" attempt. */
    enum wm_configportal_connect_state_t : uint8_t {
      WM_CP_CONNECT_IDLE = 0,
      WM_CP_CONNECT_QUEUED,
      WM_CP_CONNECT_WAITING,
      WM_CP_CONNECT_SUCCESS,
      WM_CP_CONNECT_FAILED,
    };

    enum wm_station_state_t : uint8_t {
      WM_STATION_IDLE = 0,
      WM_STATION_LOADING,
      WM_STATION_ATTEMPTING,
      WM_STATION_SWITCHING,
      WM_STATION_CONNECTED,
      WM_STATION_BACKOFF,
      WM_STATION_PORTAL,
    };

    struct wm_station_status_t {
      wm_station_state_t state = WM_STATION_IDLE;
      uint8_t activeSlot = WM_NO_STATION_PROFILE;
      uint8_t attemptedSlot = WM_NO_STATION_PROFILE;
      uint8_t configuredProfiles = 0;
      uint8_t wifiStatus = WL_IDLE_STATUS;
      bool lastConnectionWasCandidate = false;
      bool storageSaveFailed = false;
      String message = "Idle";
    };

    /** Optional notification hook; prefer getters for consumers. */
    enum wm_event_t : uint8_t {
      WM_EVENT_PORTAL_STARTED = 0,
      WM_EVENT_PORTAL_STOPPED,
      WM_EVENT_PORTAL_CONNECT_QUEUED,
      WM_EVENT_PORTAL_CONNECT_START,
      WM_EVENT_PORTAL_CONNECT_SUCCESS,
      WM_EVENT_PORTAL_CONNECT_FAILED,
      WM_EVENT_STATION_PROFILE_ATTEMPT,
      WM_EVENT_STATION_PROFILE_CONNECTED,
      WM_EVENT_STATION_PROFILE_FAILED,
      WM_EVENT_STATION_LINK_LOST,
      WM_EVENT_STATION_BACKOFF,
      WM_EVENT_STATION_PROFILES_CLEARED
    };
    using WiFiManagerEventCallback = std::function<void(wm_event_t)>;

    WiFiManager(Print& consolePort);
    WiFiManager();
    ~WiFiManager();
    void WiFiManagerInit();

    // auto connect to saved wifi, or custom, and start config portal on failures
    boolean       autoConnect();
    boolean       autoConnect(char const *apName, char const *apPassword = NULL);

    // Fixed two-profile station mode. When a store is attached, WiFiManager
    // owns profile selection and explicit connection attempts rather than the
    // platform's one saved station configuration.
    void          setStationProfileStore(WiFiManagerStationProfileStore* store);
    bool          startStationConnection(char const *apName = NULL, char const *apPassword = NULL);
    bool          startStationCandidate(const WiFiManagerStationProfiles& candidate);
    bool          startStationCandidate(const WiFiManagerStationProfiles& candidate, char const *apName, char const *apPassword = NULL);
    bool          saveStationProfiles(const WiFiManagerStationProfiles& profiles);
    void          clearStationProfiles();
    bool          isStationProfileMode() const;
    void          setStationRecoveryInterval(unsigned long intervalMs);
    const WiFiManagerStationProfiles& getStationProfiles() const;
    const wm_station_status_t& getStationStatus() const;

    //manually start the config portal, autoconnect does this automatically on connect failure
    void          startConfigPortal(); // auto generates apname
    void          startConfigPortal(char const *apName, char const *apPassword = NULL);

    //manually stop the config portal - immediately shuts down the portal
    void          stopConfigPortal();

    //manually start the web portal, autoconnect does this automatically on connect failure
    void          startWebPortal();

    //manually stop the web portal if started manually
    void          stopWebPortal();

    // Run webserver processing - must be called periodically when config portal is active
    boolean       process();

    // async scan state and cached result accessors
    void          requestAsyncScan(bool forceRefresh = false);
    const WiFiScanRuntimeState& getScanSnapshot() const { return _scan; }
    wm_scan_state_t getScanState() const { return _scan.state; }
    bool          isScanRunning() const { return _scan.state == WM_SCAN_RUNNING || _scan.state == WM_SCAN_QUEUED; }
    bool          hasValidScanResults() const { return _scan.resultsValid; }
    const WiFiScanRuntimeState& getScanRuntimeState() const { return _scan; }
    const std::vector<WiFiScanNetwork>& getScanResults() const { return _scanResultsCache; }

    // get the AP name of the config portal, so it can be used in the callback
    String        getConfigPortalSSID();
    int           getRSSIasQuality(int RSSI);

    // erase wifi credentials
    void          resetSettings();

    // reboot esp
    void          reboot();

    // disconnect wifi, without persistent saving or erasing
    bool          disconnect();

    // erase esp
    bool          erase();
    bool          erase(bool opt);

    //adds a custom parameter, returns false on failure
    bool          portalAddParameter(WiFiManagerParameter *p);

    //returns the list of Parameters
    WiFiManagerParameter** getParameters();

    // returns the Parameters Count
    int           getParametersCount();

    // SET CALLBACKS

    //called after AP mode and config portal has started
    void          setAPCallback( std::function<void(WiFiManager*)> func );

    //called after webserver has started
    void          setWebServerCallback( std::function<void()> func );

    //called when settings reset have been triggered
    void          setConfigResetCallback( std::function<void()> func );

    //called when wifi settings have been changed and connection was successful ( or setBreakAfterConfig(true) )
    void          setSaveConfigCallback( std::function<void()> func );

    //called when saving params-in-wifi or params before anything else happens (eg wifi)
    void          setPreSaveConfigCallback( std::function<void()> func );

    //called when saving params before anything else happens
    void          setPreSaveParamsCallback( std::function<void()> func );

    //called when saving either params-in-wifi or params page
    void          setSaveParamsCallback( std::function<void(WiFiManagerRequestArgs)> func );

    //called just before doing OTA update
    void          setPreOtaUpdateCallback( std::function<void()> func );

    //called when config portal is timeout
    void          setConfigPortalTimeoutCallback( std::function<void()> func );

    //sets timeout before AP,webserver loop ends and exits even if there has been no setup.
    //useful for devices that failed to connect at some point and got stuck in a webserver loop
    //in seconds setConfigPortalTimeout is a new name for setTimeout
    void          setConfigPortalTimeout(unsigned long seconds);
    void          setTimeout(unsigned long seconds); // @deprecated, alias

    //sets timeout for which to attempt connecting, useful if you get a lot of failed connects
    void          setConnectTimeout(unsigned long seconds);

    // sets number of retries for autoconnect, force retry after wait failure exit
    void          setConnectRetries(uint8_t numRetries); // default 1

    //sets timeout for which to attempt connecting on saves, useful if there are bugs in esp waitforconnectloop
    void          setSaveConnectTimeout(unsigned long seconds);

    // lets you disable automatically connecting after save from webportal
    void          setSaveConnect(bool connect = true);

    void          setLogEnabled(boolean enabled);
    void          setLogPrefix(String prefix);
    void          setLogOutput(boolean enabled, WiFiManagerLogLevel maxLevel);

    /** When non-null, log lines go here instead of the Print stream. */
    void          setLogSink(WiFiManagerLogSink* sink);
    WiFiManagerLogSink* getLogSink();

    template<typename T>
    void log(WiFiManagerLogLevel level, const char* subsystem, T&& text);

    template<typename T, typename U>
    void log(WiFiManagerLogLevel level, const char* subsystem, T&& a, U&& b);

    void log(WiFiManagerLogLevel level, const char* subsystem, const String& text);

    //set min quality percentage to include in scan, defaults to 8% if not specified
    void          setMinimumSignalQuality(int quality = 8);

    //sets a custom ip /gateway /subnet configuration
    void          setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);

    //sets config for a static IP
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);

    //sets config for a static IP with DNS
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns);

    //if this is set, it will exit after config, even if connection is unsuccessful.
    void          setBreakAfterConfig(boolean shouldBreak);


    //if this is true, remove duplicated Access Points - defaut true
    void          setRemoveDuplicateAPs(boolean removeDuplicates);

    //setter for ESP wifi.persistent so we can remember it and restore user preference, as WIFi._persistent is protected
    void          setRestorePersistent(boolean persistent);

    //if true, always show static net inputs, IP, subnet, gateway, else only show if set via setSTAStaticIPConfig
    void          setShowStaticFields(boolean alwaysShow);

    //if true, always show static dns, esle only show if set via setSTAStaticIPConfig
    void          setShowDnsFields(boolean alwaysShow);

    //if false, timeout captive portal even if a STA client connected to softAP (false), suggest disabling if captiveportal is open
    void          setAPClientCheck(boolean enabled);

    //if true, reset timeout when webclient connects (true), suggest disabling if captiveportal is open
    void          setWebPortalClientCheck(boolean enabled);

    // if true, enable autoreconnecting
    void          setWiFiAutoReconnect(boolean enabled);

    // if true, wifiscan will show percentage instead of quality icons, until we have better templating
    void          setScanDispPerc(boolean enabled);

    // if true (default) then start the config portal from autoConnect if connection failed
    void          setEnableConfigPortal(boolean enable);

    // if true (default) then stop the config portal from autoConnect when wifi is saved
    void          setDisableConfigPortal(boolean enable);

    // set a custom hostname, sets sta and ap dhcp client id for esp32, and sta for esp8266
    bool          setHostname(const char * hostname);
    bool          setHostname(String hostname);

    // set ap channel
    void          setWiFiAPChannel(int32_t channel);

    // set ap hidden
    void          setWiFiAPHidden(bool hidden); // default false

    // clean connect, always disconnect before connecting
    void          setCleanConnect(bool enable); // default false

    // ---- Portal presentation ----
    // Apply this before a portal is started. The portal response model is
    // immutable while active so async responses never observe partial UI state.
    bool          setPortalConfig(const WiFiManagerPortalConfig& config);

    void          portalSetPageInfoVisible(bool visible);
    void          portalSetPageUpdateVisible(bool visible);
    void          portalSetPageSetupVisible(bool visible);

    void          portalSetActionEraseVisible(bool visible);
    void          portalSetActionRestartVisible(bool visible);
    void          portalSetActionExitVisible(bool visible);
    void          portalSetActionCloseCaptiveVisible(bool visible);
    void          portalSetActionBackVisible(bool visible);

    void          portalSetLayoutParamsLocation(PortalParamsLocation location);

    void          portalSetBehaviorCaptivePortalEnabled(bool enabled);
    void          portalSetBehaviorConnectOnSave(bool enabled);
    void          portalSetBehaviorExitAllowed(bool allowed);
    void          portalSetBehaviorConnectTimeoutSeconds(unsigned long seconds);
    void          portalSetBehaviorPortalTimeoutSeconds(unsigned long seconds);
    void          portalSetBehaviorAutoReconnect(bool enabled);
    void          portalSetBehaviorApClientCheck(bool enabled);
    void          portalSetBehaviorWebClientCheck(bool enabled);

    void          portalSetFieldPasswordPlaceholderMode(PortalPasswordPlaceholderMode mode);
    void          portalSetFieldStaticIpVisibility(PortalFieldVisibility visibility);
    void          portalSetFieldStaticDnsVisibility(PortalFieldVisibility visibility);

    void          portalClearParameters();
    void          portalAddInfoSection(const PortalInfoSection& section);
    void          portalClearInfoSections();
    void          portalAddHomeCard(const PortalHomeCard& card);
    void          portalClearHomeCards();

    // get last connection result, including autoconnect and portal credential-save attempts
    uint8_t       getLastConxResult();

    // get a status as string
    String        getWLStatusString(uint8_t status);
    String        getWLStatusString();

    // get wifi mode as string
    String        getModeString(uint8_t mode);

    // check if the module has a saved ap to connect to
    bool          getWiFiIsSaved();

    // helper to get saved password, if persistent get stored, else get current if connected
    String        getWiFiPass(bool persistent = true);

    // helper to get saved ssid, if persistent get stored, else get current if connected
    String        getWiFiSSID(bool persistent = true);

    // debug output the softap config
    void          debugSoftAPConfig();

    // debug output platform info and versioning
    void          debugPlatformInfo();

    // helper for html
    String        htmlEntities(String str, bool whitespace = false);

    // set the country code for wifi settings, CN
    void          setCountry(String cc);


    // get default ap esp uses , esp_chipid etc
    String        getDefaultAPName();

    // set the WiFi SSID prefix for default AP name, default platform-specific (ESP/ESP32/WM)
    void          setWiFiSSIDPrefix(String prefix);

    // set port of webserver, 80
    void          setHttpPort(uint16_t port);

    // check if config portal is active (true)
    bool          getConfigPortalActive() const;

    /** True once the config portal has been entered in this power-on / runtime session. */
    bool          hasEnteredConfigPortal() const;
    /** Summarized state for the last portal submit of WiFi credentials. */
    wm_configportal_connect_state_t getConfigPortalConnectState() const;
    bool          isConfigPortalConnectPending() const;
    bool          didConfigPortalConnectSucceed() const;
    bool          didConfigPortalConnectFail() const;
    uint8_t       getConfigPortalConnectStatus() const;
    String        getConfigPortalConnectMessage() const;
    void          setEventCallback(WiFiManagerEventCallback cb);

    // check if web portal is active (true)
    bool          getWebPortalActive();

    // to preload autoconnect for test fixtures or other uses that skip esp sta config
    bool          preloadWiFi(String ssid, String pass);

    // get hostname helper
    String        getWiFiHostname();

    // get server instance (for testing)
    AsyncWebServer* getServer();

    // get DNS server instance (for testing)
    DNSServer* getDNSServer();

  public:
  // Define WiFiManagerRequestArgs here after AsyncWebServer is available
  class WiFiManagerRequestArgs {
  public:
      std::unordered_map<std::string, std::string> args;

      // Constructor - builds from AsyncWebServerRequest
      WiFiManagerRequestArgs(AsyncWebServerRequest* request) {
          if (request) {
              size_t paramCount = request->params();
              for (size_t i = 0; i < paramCount; i++) {
                  const AsyncWebParameter* p = request->getParam(i);
                  args[std::string(p->name().c_str())] = std::string(p->value().c_str());
              }
          }
      }

      // Default constructor for tests and manually assembled argument sets
      WiFiManagerRequestArgs() {}

      // Check if argument exists
      bool hasArg(const char* name) const {
          return args.find(std::string(name)) != args.end();
      }

      bool hasArg(const String& name) const {
          return hasArg(name.c_str());
      }

      // Get argument value as String
      String getArg(const char* name, const String& defaultValue = "") const {
          auto it = args.find(std::string(name));
          if (it != args.end()) {
              return String(it->second.c_str());
          }
          return defaultValue;
      }

      String getArg(const String& name, const String& defaultValue = "") const {
          return getArg(name.c_str(), defaultValue);
      }

      // Type conversion helpers
      int getArgAsInt(const char* name, int defaultValue = 0) const {
          String value = getArg(name);
          return value.length() > 0 ? value.toInt() : defaultValue;
      }

      float getArgAsFloat(const char* name, float defaultValue = 0.0f) const {
          String value = getArg(name);
          return value.length() > 0 ? value.toFloat() : defaultValue;
      }

      bool getArgAsBool(const char* name, bool defaultValue = false) const {
          String value = getArg(name);
          if (value.length() == 0) return defaultValue;
          return value == "1" || value.equalsIgnoreCase("true") ||
                 value.equalsIgnoreCase("on") || value.equalsIgnoreCase("yes");
      }

      size_t count() const {
          return args.size();
      }
  };

  friend class WiFiManagerServer;
  friend class WiFiManagerHandlers;

  protected:
    // vars
    std::unique_ptr<WiFiManagerServer> _serverManager;

    // ip configs @todo struct ?
    IPAddress     _ap_static_ip;
    IPAddress     _ap_static_gw;
    IPAddress     _ap_static_sn;
    IPAddress     _sta_static_ip;
    IPAddress     _sta_static_gw;
    IPAddress     _sta_static_sn;
    IPAddress     _sta_static_dns;

    unsigned long _configPortalStart      = 0; // ms config portal start time (updated for timeouts)
    unsigned long _webPortalAccessed      = 0; // ms last web access time
    uint8_t       _lastconxresult         = WL_IDLE_STATUS; // store last result when doing connect operations
    int           _numNetworks            = 0; // init index for numnetworks wifiscans
    unsigned long _lastscan               = 0; // ms for timing wifi scans
    unsigned long _startconn              = 0; // ms for timing wifi connects

    // async scan state management
    WiFiScanRuntimeState _scan;
    std::vector<WiFiScanNetwork> _scanResultsCache;
    bool _scanLifecycleBlocked = false;

    // async reboot/abort scheduling
    bool          _rebootScheduled        = false; // flag for scheduled reboot
    unsigned long _rebootTime             = 0; // ms when reboot should occur
    bool          _abortScheduled        = false; // flag for scheduled abort
    unsigned long _abortTime              = 0; // ms when abort should occur

    // defaults
    const uint8_t  DNS_PORT               = 53;
    const unsigned long REBOOT_DELAY_MS   = 1000; // delay before reboot after response sent
    const unsigned long EXIT_DELAY_MS     = 2000; // delay before abort after response sent
    const unsigned long ERASE_REBOOT_DELAY_MS = 2000; // delay before reboot after erase response sent
    String        _apName                 = "no-net";
    String        _apPassword             = "";
    String        _ssid                   = ""; // var temp ssid
    String        _pass                   = ""; // var temp psk
    String        _defaultssid            = ""; // preload ssid
    String        _defaultpass            = ""; // preload pass

    WiFiManagerStationProfileStore* _stationProfileStore = nullptr;
    WiFiManagerStationProfiles _stationProfiles;
    WiFiManagerStationProfiles _stationCandidate;
    wm_station_status_t _stationStatus;
    bool          _stationProfilesLoaded  = false;
    bool          _stationCandidateActive = false;
    bool          _stationCandidateFromPortal = false;
    bool          _stationEverConnected   = false;
    uint8_t       _stationAttemptMask     = 0;
    uint8_t       _stationPendingSlot    = WM_NO_STATION_PROFILE;
    unsigned long _stationNextAttemptAt  = 0;
    unsigned long _stationAttemptStartedAt = 0;
    unsigned long _stationBackoffStartedAt = 0;
    unsigned long _stationRecoveryInterval = 5000UL;
    String        _stationPortalApName    = "";
    String        _stationPortalApPassword = "";

    // options flags
    unsigned long _configPortalTimeout    = 0; // ms close config portal loop if set (depending on  _cp/webClientCheck options)
    unsigned long _connectTimeout         = 0; // ms stop trying to connect to ap if set
    unsigned long _saveTimeout            = 0; // ms stop trying to connect to ap on saves, in case bugs in esp waitforconnectresult

    WiFiMode_t    _usermode               = WIFI_STA; // Default user mode
    String        _wifissidprefix         =
#ifdef ESP8266
        "ESP"
#elif defined(ESP32)
        "ESP32"
#else
        "WM"
#endif
        ; // auto apname prefix prefix+chipid
    int           _cpclosedelay           = 2000; // delay before portal save completes; prevents captive portal from closing too fast.
    bool          _cleanConnect           = false; // disconnect before connect in connectwifi, increases stability on connects
    bool          _connectonsave          = true; // connect to wifi when saving creds
    bool          _disableSTA             = false; // disable sta when starting ap, always
    bool          _disableSTAConn         = true;  // disable sta when starting ap, if sta is not connected ( stability )
    bool          _channelSync            = false; // use same wifi sta channel when starting ap
    int32_t       _apChannel              = 0; // default channel to use for ap, 0 for auto
    bool          _apHidden               = false; // store softap hidden value
    uint16_t      _httpPort               = 80; // port for webserver
    // uint8_t       _retryCount             = 0; // counter for retries, probably not needed if synchronous
    uint8_t       _connectRetries         = 1; // number of sta connect retries, force reconnect, wait loop (connectimeout) does not always work and first disconnect bails
    bool          _aggresiveReconn        = false; // use an agrressive reconnect strategy, WILL delay conxs
                                                   // on some conn failure modes will add delays and many retries to work around esp and ap bugs, ie, anti de-auth protections
                                                   // https://github.com/tzapu/WiFiManager/issues/1067
    bool          _allowExit              = true; // allow exit/abort calls - if false, user exit/abort calls will be ignored including cptimeout

    #ifdef ESP32
    wifi_event_id_t wm_event_id           = 0;
    static uint8_t _lastconxresulttmp; // tmp var for esp32 callback
    #endif

    #ifndef WL_STATION_WRONG_PASSWORD
    uint8_t WL_STATION_WRONG_PASSWORD     = 7; // @kludge define a WL status for wrong password
    #endif

    // parameter options
    int           _minimumQuality         = -1;    // filter wifiscan ap by this rssi
    int           _staShowStaticFields    = 0;     // ternary 1=always show static ip fields, 0=only if set, -1=never(cannot change ips via web!)
    int           _staShowDns             = 0;     // ternary 1=always show dns, 0=only if set, -1=never(cannot change dns via web!)
    boolean       _removeDuplicateAPs     = true;  // remove dup aps from wifiscan
    PortalPasswordPlaceholderMode _portalPasswordPlaceholderMode = PortalPasswordPlaceholderMode::Masked;
    boolean       _shouldBreakAfterConfig = false; // stop configportal on save failure
    boolean       _enableCaptivePortal    = true;  // enable captive portal redirection
    boolean       _userpersistent         = true;  // users preffered persistence to restore
    boolean       _wifiAutoReconnect      = true;  // there is no platform getter for this, we must assume its true and make it so
    boolean       _apClientCheck          = false; // keep cp alive if ap have station
    boolean       _webClientCheck         = true;  // keep cp alive if web have client
    boolean       _scanDispOptions        = false; // show percentage in scans not icons
    boolean       _enableConfigPortal     = true;  // FOR autoconnect - start config portal if autoconnect failed
    boolean       _disableConfigPortal    = true;  // FOR autoconnect - stop config portal if cp wifi save
    String        _hostname               = "";    // hostname for esp8266 for dhcp, and or MDNS

    // Grouped portal presentation / customization (see portal APIs and JSON bootstrap)
    PortalBrandState            _portalBrand;
    WiFiManagerPortalTheme      _portalTheme;
    String                      _portalThemeStyle;
    PortalPageState             _portalPages;
    PortalActionState           _portalActions;
    PortalLayoutState           _portalLayout;
    PortalStructuredExtrasState _portalStructured;

    bool canChangePortalPresentation() const;
    bool isPortalThemeValid(const WiFiManagerPortalTheme& theme) const;
    void rebuildPortalThemeStyle();

    // internal options

    // wifiscan notes
    // currently disabled due to issues with caching, sometimes first scan is empty esp32 wifi not init yet race, or portals hit server nonstop flood
    // The following are background wifi scanning optimizations
    // experimental to make scans faster, preload scans after starting cp, and visiting home page, so when you click wifi its already has your list
    // ideally we would add async and xhr here but I am holding off on js requirements atm
    // might be slightly buggy since captive portals hammer the home page, @todo workaround this somehow.
    // cache time helps throttle this
    // async enables asyncronous scans, so they do not block anything
    // the refresh button bypasses cache
    // no aps found is problematic as scans are always going to want to run, leading to page load delays
    //
    // These settings really only make sense with _preloadwifiscan true
    // but not limited to, we could run continuous background scans on various page hits, or xhr hits
    // which would be better coupled with asyncscan
    // atm preload is only done on root hit and startcp
    //
    // preload scanning causes AP to delay showing for users, but also caches and lets the cp load faster once its open
    //  my scan takes 7-10 seconds
public:
    boolean       _preloadwifiscan        = true;  // begin one asynchronous scan as the portal starts
    unsigned int  _scancachetime          = 30000; // ms cache time for preload scans

protected:

    boolean       _autoforcerescan        = false;  // automatically force rescan if scan networks is 0, ignoring cache

    boolean       _disableIpFields        = false; // modify function of setShow_X_Fields(false), forces ip fields off instead of default show if set, eg. _staShowStaticFields=-1

    String        _wificountry            = "";  // country code, @todo define in strings lang

    // wrapper functions for handling setting and unsetting persistent for now.
    bool          esp32persistent         = false;
    bool          _hasBegun               = false; // flag wm loaded,unloaded
    void          _begin();
    void          _end();

    void          setupConfigPortal();
    bool          shutdownConfigPortal();
    bool          setupHostname(bool restart);

#ifdef NO_EXTRA_4K_HEAP
    boolean       _tryWPS                 = false; // try WPS on save failure, unsupported
    void          startWPS();
#endif

    bool          startAP();
    void          setupDNSD();
    enum class wm_autoconnect_result_t : uint8_t {
      connected = 0,
      no_credentials,
      failed,
      fatal
    };
    wm_autoconnect_result_t attemptAutoConnect();

    uint8_t       connectWifi(String ssid, String pass, bool connect = true);
    bool          setSTAConfig();
    bool          wifiConnectDefault();
    bool          wifiConnectNew(String ssid, String pass,bool connect = true);

    void          processStationController();
    bool          beginStationProfile(uint8_t slot);
    void          queueStationProfile(uint8_t slot);
    void          beginStationCycle(bool preferLastSuccessful);
    void          handleStationAttemptFailure(uint8_t status, const String& message);
    void          handleStationConnectionSuccess();
    void          completePortalStationAttempt(bool success, uint8_t status, const String& message);
    void          acknowledgePortalConnectHandoff();
    void          enterStationPortal();
    bool          hasUsableStationConnection() const;
    bool          isStationProfileEnabled(const WiFiManagerStationProfiles& profiles, uint8_t slot) const;
    uint8_t       configuredStationProfileCount(const WiFiManagerStationProfiles& profiles) const;
    uint8_t       chooseStationProfile(const WiFiManagerStationProfiles& profiles, bool preferLastSuccessful) const;
    const WiFiManagerStationProfiles& stationProfilesForAttempt() const;
    WiFiManagerStationProfiles& stationProfilesForAttempt();
    bool          validateStationProfiles(const WiFiManagerStationProfiles& profiles) const;
    unsigned long stationAttemptTimeout() const;

    uint8_t       waitForConnectResult();
    uint8_t       waitForConnectResult(uint32_t timeout);
    void          updateConxResult(uint8_t status);

    // Config portal lifecycle (handlers moved to WiFiManagerHandlers)
    boolean       configPortalHasTimeout();
    uint8_t       processConfigPortal();


    // wifi platform abstractions
    bool          WiFi_Mode(WiFiMode_t m);
    bool          WiFi_Mode(WiFiMode_t m,bool persistent);
    bool          WiFi_Disconnect();
    bool          WiFi_enableSTA(bool enable);
    bool          WiFi_enableSTA(bool enable,bool persistent);
    bool          WiFi_eraseConfig();
    uint8_t       WiFi_softap_num_stations();
    bool          WiFi_hasAutoConnect();
    void          WiFi_autoReconnect();
    String        WiFi_SSID(bool persistent = true) const;
    String        WiFi_psk(bool persistent = true) const;
    bool          WiFi_scanNetworks();
    bool          WiFi_scanNetworks(bool force); // Always async - returns false if scan started but not complete
    bool          WiFi_scanNetworks(unsigned int cachetime);
    void          WiFi_scanComplete(int networksFound);
    void          processScan();
    void          processPortalConnect();
    void          queuePortalConnect(const String& ssid, const String& pass);
    void          emitPortalEvent(wm_event_t event);
    bool          shouldScheduleScan(unsigned int cachetime,
                                     wm_scan_schedule_reason_t reason,
                                     bool forceRefresh = false) const;
    void          scheduleScan(wm_scan_schedule_reason_t reason, bool forceRefresh = false);
    bool          startAsyncScan();
    void          finalizeAsyncScan(int networksFound);
    void          failAsyncScan(wm_scan_state_t state, int scanResult = WIFI_SCAN_FAILED);
    void          resetAsyncScan(bool clearResults);
    void          invalidateScanResults();
    bool          hasFreshScanResults(unsigned int cachetime) const;
    bool          canRunAsyncScan() const;
    void          cacheScanResults(int networksFound);
    bool          WiFiSetCountry();

    #ifdef ESP32

    // check for arduino or system event system, handle esp32 arduino v2 and IDF
    #if defined(ESP_ARDUINO_VERSION) && defined(ESP_ARDUINO_VERSION_VAL)

        #define WM_ARDUINOVERCHECK ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0)
        #define WM_ARDUINOVERCHECK_204 ESP_ARDUINO_VERSION <= ESP_ARDUINO_VERSION_VAL(2, 0, 5)

        #ifdef WM_ARDUINOVERCHECK
            #define WM_ARDUINOEVENTS
        #else
            #define WM_NOSOFTAPSSID
            #define WM_NOCOUNTRY
        #endif

        #ifdef WM_ARDUINOVERCHECK_204
            #define WM_DISCONWORKAROUND
        #endif

    #else
        #define WM_NOCOUNTRY
    #endif

    #ifdef WM_NOCOUNTRY
        #warning "ESP32 set country unavailable"
    #endif


    #ifdef WM_ARDUINOEVENTS
        void   WiFiEvent(WiFiEvent_t event, arduino_event_info_t info);
    #else
        void   WiFiEvent(WiFiEvent_t event, system_event_info_t info);
    #endif
    #endif

    #ifdef UNIT_TEST
  public:
    void          wmTestForceScanState(wm_scan_state_t state) { _scan.state = state; }
    void          wmTestSetScanStartedAt(unsigned long startedAt) { _scan.startedAt = startedAt; }
    void          wmTestSetScanTimeoutMs(unsigned long timeoutMs) { _scan.timeoutMs = timeoutMs; }
    void          wmTestInjectScanResults(const std::vector<WiFiScanNetwork>& results) {
      _scanResultsCache = results;
      _numNetworks = static_cast<int>(results.size());
      _scan.visibleNetworkCount = _numNetworks;
      _scan.resultsValid = true;
      _scan.state = WM_SCAN_COMPLETE;
      _lastscan = millis();
      _scan.finishedAt = _lastscan;
    }
    void          wmTestSetScanCompletionPending(int completionResult) {
      _scan.completionPending = true;
      _scan.completionResult = completionResult;
    }
    void          wmTestSetPortalActive(bool active) {
      configPortalActive = active;
      // Keep the test helper aligned with real portal startup so timeout-based
      // assertions are anchored to "now" instead of device boot.
      if (active) {
        _configPortalStart = millis();
      }
    }
    void          wmTestSetPortalConnectSuccess(const String& message, const String& stationIp, uint8_t status = WL_CONNECTED) {
      _cpConnectState = wm_cp_connect_state_t::success;
      _cpConnectMessage = message;
      _cpConnectStationIp = stationIp;
      _cpConnectStatus = status;
    }
    void          wmTestCompleteProfilePortalConnectionSuccess();
    void          wmTestSetPortalConnectFailure(const String& message, uint8_t status = WL_CONNECT_FAILED) {
      _cpConnectState = wm_cp_connect_state_t::failed;
      _cpConnectMessage = message;
      _cpConnectStationIp = "";
      _cpConnectStatus = status;
    }
    void          wmTestSetConnectPending(bool active);
    void          wmTestSetScanLifecycleBlocked(bool blocked) { _scanLifecycleBlocked = blocked; }
    void          wmTestSetScanGenerations(uint32_t generation, uint32_t runningGeneration, uint32_t completionGeneration) {
      _scan.generation = generation;
      _scan.runningGeneration = runningGeneration;
      _scan.completionGeneration = completionGeneration;
    }
    void          wmTestClearScanResults() { resetAsyncScan(true); }
    #endif

  protected:
    /**
     * Internal portal-connect workflow. Distinct from wm_configportal_connect_state_t (unscoped
     * public enum) — enum class avoids name clashes with WM_CP_CONNECT_* in the public API.
     */
    enum class wm_cp_connect_state_t : uint8_t {
      idle = 0,
      queued,
      delaying,
      starting,
      waiting,
      success_waiting_close,
      success,
      failed
    };

    bool                    _hasEnteredConfigPortal = false;
    wm_cp_connect_state_t   _cpConnectState         = wm_cp_connect_state_t::idle;
    String        _cpConnectSsid;
    String        _cpConnectPass;
    String        _cpConnectMessage;
    uint8_t       _cpConnectStatus = WL_IDLE_STATUS;
    unsigned long _cpConnectQueuedAt = 0;
    unsigned long _cpConnectStartedAt = 0;
    unsigned long _cpConnectDelayUntil = 0;
    unsigned long _cpConnectTimeoutMs = 0;
    unsigned long _cpConnectCloseAt = 0;
    String        _cpConnectStationIp;
    WiFiManagerEventCallback _eventCallback = nullptr;

    //helpers (rendering methods moved to WiFiManagerHandlers)
    boolean       isIp(String str);
    String        toStringIp(IPAddress ip);
    boolean       validApPassword();
    String        encryptionTypeStr(uint8_t authmode);

    // flags
    boolean       abort               = false;
    boolean       reset               = false;
    boolean       configPortalActive  = false;


    // these are state flags for portal mode, we are either in webportal mode(STA) or configportal mode(AP)
    // these are mutually exclusive as STA+AP mode is not supported due to channel restrictions and stability
    // if we decide to support this, these checks will need to be replaced with something client aware to check if client origin is ap or web
    // These state checks are critical and used for internal function checks
    boolean       webPortalActive     = false;
    boolean       storeSTAmode        = true; // option store persistent STA mode in connectwifi
    int           timer               = 0;    // timer for debug throttle for numclients, and portal timeout messages

    // WiFiManagerParameter
    int         _paramsCount          = 0;
    int         _max_params;
    WiFiManagerParameter** _params    = NULL;

    boolean       _logEnabled = true;
    String        _logPrefix = "*wm:";
    WiFiManagerLogLevel _logTagMinLevel = WiFiManagerLogLevel::Debug;

#ifndef WM_NO_LOG
    WiFiManagerLogLevel _runtimeMaxLevel = static_cast<WiFiManagerLogLevel>((uint8_t)WM_LOG_LEVEL);
#else
    WiFiManagerLogLevel _runtimeMaxLevel = WiFiManagerLogLevel::Silent;
#endif

    // @todo use DEBUG_ESP_PORT ?
    #ifdef WM_DEBUG_PORT
    Print&        _logPort = WM_DEBUG_PORT;
    #else
    Print&        _logPort = Serial;
    #endif

    void          emitLogImpl(WiFiManagerLogLevel level, const char* subsystem, const String& textA, const String& textB);

    // callbacks
    // @todo use cb list (vector) maybe event ids, allow no return value
    std::function<void(WiFiManager*)> _apcallback;
    std::function<void()> _webservercallback;
    std::function<void()> _savewificallback;
    std::function<void()> _presavewificallback;
    std::function<void()> _presaveparamscallback;
    std::function<void(WiFiManagerRequestArgs)> _saveparamscallback;
    std::function<void()> _resetcallback;
    std::function<void()> _preotaupdatecallback;
    std::function<void()> _configportaltimeoutcallback;

    WiFiManagerLogSink* _logSink = nullptr;

    template <class T>
    auto optionalIPFromString(T *obj, const char *s) -> decltype(  obj->fromString(s)  ) {
      return  obj->fromString(s);
    }
    auto optionalIPFromString(...) -> bool {
      return false;
    }

};

#include "WiFiManagerLogTemplates.h"

#endif

#endif
