/**
 * WiFiManagerLogTemplates.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * Template implementations for WiFiManager::log(...). Included at the end of WiFiManager.h.
 */

#ifndef WiFiManagerLogTemplates_h
#define WiFiManagerLogTemplates_h

#include "WiFiManagerLogLevel.h"
#include <IPAddress.h>
#include <WString.h>

#if defined(ESP8266) || defined(ESP32)

inline String wmLogStringifyArg(const __FlashStringHelper* v) {
    return String(v);
}

inline String wmLogStringifyArg(const IPAddress& v) {
    return v.toString();
}

template<typename T>
inline String wmLogStringifyArg(const T& v) {
    return String(v);
}

template<typename T>
inline void WiFiManager::log(WiFiManagerLogLevel level, const char* subsystem, T&& text) {
#ifndef WM_NO_LOG
    emitLogImpl(level, subsystem, wmLogStringifyArg(text), String());
#endif
}

template<typename T, typename U>
inline void WiFiManager::log(WiFiManagerLogLevel level, const char* subsystem, T&& a, U&& b) {
#ifndef WM_NO_LOG
    emitLogImpl(level, subsystem, wmLogStringifyArg(a), wmLogStringifyArg(b));
#endif
}

#endif // ESP8266 || ESP32

#endif // WiFiManagerLogTemplates_h
