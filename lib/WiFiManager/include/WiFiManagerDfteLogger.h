/**
 * WiFiManagerDfteLogger.h
 *
 * Optional bridge from DFTE's logging hooks to WiFiManager::DEBUG_WM.
 * Enabled only when the sketch / library build defines WM_DFTE_LOGGING.
 */

#ifndef WiFiManagerDfteLogger_h
#define WiFiManagerDfteLogger_h

#if defined(ESP8266) || defined(ESP32)

#include <DeviceFrameworkTemplateEngineDebug.h>
#include "WiFiManager.h"

/**
 * Forwards DeviceFrameworkTemplateEngineLogger calls to WiFiManager's
 * existing DEBUG_WM pipeline (respects _debug, _debugLevel, _debugPrefix, _debugPort).
 */
class WiFiManagerDfteLogger : public DeviceFrameworkTemplateEngineLogger {
public:
    explicit WiFiManagerDfteLogger(WiFiManager* wm) : _wm(wm) {}

    void error(const String& msg) override {
        logLine(WM_DEBUG_ERROR, msg);
    }
    void warn(const String& msg) override {
        logLine(WM_DEBUG_NOTIFY, msg);
    }
    void info(const String& msg) override {
        logLine(WM_DEBUG_VERBOSE, msg);
    }
    void debug(const String& msg) override {
        logLine(WM_DEBUG_DEV, msg);
    }

private:
    WiFiManager* _wm;

    void logLine(wm_debuglevel_t level, const String& msg) {
        if (!_wm) {
            return;
        }
        String line;
        line.reserve(static_cast<unsigned>(msg.length()) + 12u);
        line += F("[DFTE] ");
        line += msg;
        _wm->DEBUG_WM(level, line);
    }
};

#endif // ESP8266 || ESP32

#endif // WiFiManagerDfteLogger_h
