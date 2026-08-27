/**
 * WiFiManagerHandlers.h
 *
 * HTTP request handlers for WiFiManager: single HTML shell + JSON APIs.
 *
 * @author alexhopeoconnor
 * @license MIT
 */

#ifndef WiFiManagerHandlers_h
#define WiFiManagerHandlers_h

#if defined(ESP8266) || defined(ESP32)

#include <ESPAsyncWebServer.h>
#include <memory>
#include "WiFiManager.h"

#ifndef WM_TEMPLATE_REGISTRY_CAPACITY
#define WM_TEMPLATE_REGISTRY_CAPACITY 16
#endif

// Portal UI customization is driven by WiFiManager `portal*` APIs and JSON (/api/...) — not by mutating template
// placeholder registries.

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

    void handleRoot(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
    void handleUpdating(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdateDone(AsyncWebServerRequest *request);

    void handleApiBootstrap(AsyncWebServerRequest *request);
    void handleApiWifiScanStatus(AsyncWebServerRequest *request);
    void handleApiWifiScan(AsyncWebServerRequest *request);
    void handleApiWifiMeta(AsyncWebServerRequest *request);
    void handleApiWifiSave(AsyncWebServerRequest *request);
    void handleApiWifiConnectStatus(AsyncWebServerRequest *request);
    void handleApiParamsGet(AsyncWebServerRequest *request);
    void handleApiParamsSave(AsyncWebServerRequest *request);
    void handleApiInfo(AsyncWebServerRequest *request);
    void handleApiStatus(AsyncWebServerRequest *request);
    void handleApiDeviceRestart(AsyncWebServerRequest *request);
    void handleApiDeviceErase(AsyncWebServerRequest *request, boolean optionalErase = false);
    void handleApiPortalClose(AsyncWebServerRequest *request);
    void handleApiPortalExit(AsyncWebServerRequest *request);

    boolean captivePortal(AsyncWebServerRequest *request);
    void stopCaptivePortal();

    /** True when the HTTP Host header does not match the portal’s canonical host:port (captive redirect needed). */
    static bool shouldRedirectCaptiveForHost(const String& requestHost, const String& serverLocWithPort);

    void doParamSave(WiFiManager::WiFiManagerRequestArgs requestArgs);

    /** Portal bootstrap JSON (same payload as GET /api/bootstrap). Public for tests and host integration. */
    String buildPortalBootstrapJson();
    /** Same JSON body as GET /api/wifi/meta (tests + embedding). */
    String buildApiWifiMetaJson();
    /** Same JSON body as GET /api/info (tests + embedding). */
    String buildApiInfoJson();
    /** Same JSON body as GET /api/params (tests + embedding). */
    String buildApiParamsGetJson();
    /** Same JSON body as GET /api/status (tests + embedding). */
    String buildApiStatusJson();
    /** GET /api/wifi/connect-status (portal connect progress). */
    String buildApiWifiConnectStatusJson();

    /** Fixed JSON bodies for POST action endpoints (single source for handlers + tests). */
    static String jsonApiWifiScanAccepted();
    static String jsonApiDeviceRestartScheduled();
    static String jsonApiParamsSaveOk();
    static String jsonApiPortalCloseOk();
    static String jsonApiPortalExitOk();
    static String jsonApiPortalExitForbidden();
    static String jsonApiOtaUpdateSuccess();
    static String jsonApiEraseResponse(boolean success);

  private:
    WiFiManager* _wm;
    void collectVisibleScanResults(std::vector<const WiFiManager::WiFiScanNetwork*>& networks);
    void appendVisibleScanResultsJson(String& json, const std::vector<const WiFiManager::WiFiScanNetwork*>& networks);

    void applyWifiAndParamsFromRequest(AsyncWebServerRequest *request);
    bool buildStationProfilesFromRequest(AsyncWebServerRequest *request, WiFiManagerStationProfiles& profiles);
    void buildPlainStatusSummary(String& out);
    void appendPortalJsonStaticFields(String& json, bool& first);
    void appendPortalJsonCustomParams(String& json, bool& first);

    String composePortalStylesheet() const;
    void appendPortalExtraInfoSectionsJson(String& json, bool& first);
    void appendPortalExtraHomeCardsJson(String& json, bool& first);

    void appendJsonKvItem(String& json, bool& first, const char* key, const String& label, const String& value);
    void appendOneInfoItemForId(String& json, bool& first, const char* id);
    void appendInfoSectionFromIds(String& json, const char* const* ids, size_t count, bool& first);

    void appendApiInfoActionsJson(String& json);

    static void sendApiJson(AsyncWebServerRequest *request, int code, const String& json);
};

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerHandlers_h
