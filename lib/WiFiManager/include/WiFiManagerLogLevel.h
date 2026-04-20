/**
 * WiFiManagerLogLevel.h
 *
 * Compile-time and runtime log levels for WiFiManager. Higher numeric values
 * mean more verbose output. Matches the old 0..5 tier model for build flags.
 */

#ifndef WiFiManagerLogLevel_h
#define WiFiManagerLogLevel_h

#include <Arduino.h>

/** Default subsystem tag for core WiFiManager messages (macro for C++11 embedded toolchains). */
#define kWiFiMgrLogSubsystem "WM"

enum class WiFiManagerLogLevel : uint8_t {
    Silent = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
};

/** Full line delivered to Print or WiFiManagerLogSink (prefix and tags included). */
struct WiFiManagerLogMessage {
    WiFiManagerLogLevel level;
    const char* subsystem;
    String line;
};

#endif
