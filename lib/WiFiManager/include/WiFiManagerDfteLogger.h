/**
 * WiFiManagerDfteLogger.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * Optional bridge from DFTE's logging hooks to WiFiManager::log.
 * Enabled only when the sketch / library build defines WM_DFTE_LOGGING.
 */

#ifndef WiFiManagerDfteLogger_h
#define WiFiManagerDfteLogger_h

#if defined(ESP8266) || defined(ESP32)

#include <DeviceFrameworkTemplateEngineDebug.h>
#include "WiFiManager.h"

/**
 * Forwards DeviceFrameworkTemplateEngineLogger calls to WiFiManager logging
 * with subsystem WM/DFTE.
 */
class WiFiManagerDfteLogger : public DeviceFrameworkTemplateEngineLogger {
public:
    explicit WiFiManagerDfteLogger(WiFiManager* wm) : _wm(wm) {}

    void error(const String& msg) override { logLine(WiFiManagerLogLevel::Error, msg); }
    void warn(const String& msg) override { logLine(WiFiManagerLogLevel::Warn, msg); }
    void info(const String& msg) override { logLine(WiFiManagerLogLevel::Debug, msg); }
    void debug(const String& msg) override { logLine(WiFiManagerLogLevel::Trace, msg); }

private:
    WiFiManager* _wm;

    void logLine(WiFiManagerLogLevel level, const String& msg) {
        if (!_wm) {
            return;
        }
        _wm->log(level, "WM/DFTE", msg);
    }
};

#endif // ESP8266 || ESP32

#endif // WiFiManagerDfteLogger_h
