/**
 * WiFiManagerUtils.cpp
 * Implementation of pure utility functions for WiFiManager
 *
 * @author alexhopeoconnor
 * @license MIT
 */

#include "WiFiManagerUtils.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

namespace WiFiManagerUtils {

// -----------------------------------------------------------------------------------------------
// WIFI STATUS ARRAYS

const char * const WIFI_STA_STATUS[] PROGMEM = {
  "WL_IDLE_STATUS",     // 0 STATION_IDLE
  "WL_NO_SSID_AVAIL",   // 1 STATION_NO_AP_FOUND
  "WL_SCAN_COMPLETED",  // 2
  "WL_CONNECTED",       // 3 STATION_GOT_IP
  "WL_CONNECT_FAILED",  // 4 STATION_CONNECT_FAIL, STATION_WRONG_PASSWORD(NI)
  "WL_CONNECTION_LOST", // 5
  "WL_DISCONNECTED",    // 6
  "WL_STATION_WRONG_PASSWORD" // 7 KLUDGE
};

#ifdef ESP32
const char * const AUTH_MODE_NAMES[] PROGMEM = {
    "OPEN",
    "WEP",
    "WPA_PSK",
    "WPA2_PSK",
    "WPA_WPA2_PSK",
    "WPA2_ENTERPRISE",
    "MAX"
};
#elif defined(ESP8266)
const char * const AUTH_MODE_NAMES[] PROGMEM = {
    "",
    "",
    "WPA_PSK",      // 2 ENC_TYPE_TKIP
    "",
    "WPA2_PSK",     // 4 ENC_TYPE_CCMP
    "WEP",          // 5 ENC_TYPE_WEP
    "",
    "OPEN",         //7 ENC_TYPE_NONE
    "WPA_WPA2_PSK", // 8 ENC_TYPE_AUTO
};
#endif

const char* const WIFI_MODES[] PROGMEM = { "NULL", "STA", "AP", "STA+AP" };

// -----------------------------------------------------------------------------------------------
// STRING CONVERSION UTILITIES

String getStatusString(uint8_t status) {
  if(status <= 7) return FPSTR(WIFI_STA_STATUS[status]);
  return F("Unknown");
}


String getModeString(uint8_t mode) {
  if(mode <= 3) return FPSTR(WIFI_MODES[mode]);
  return F("Unknown");
}

String getEncryptionString(uint8_t authmode) {
  return FPSTR(AUTH_MODE_NAMES[authmode]);
}

// -----------------------------------------------------------------------------------------------
// HTML UTILITIES

String htmlEntities(const String& str, bool whitespace) {
  String result = str;
  result.replace("&","&amp;");
  result.replace("<","&lt;");
  result.replace(">","&gt;");
  result.replace("'","&#39;");
  if(whitespace) result.replace(" ","&#160;");
  return result;
}

// -----------------------------------------------------------------------------------------------
// IP/STRING UTILITIES

bool isValidIP(const String& str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

String ipToString(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

bool isValidAPPassword(const String& password) {
  if (password.length() < 8 || password.length() > 63) {
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------------------------
// HELPER CALCULATIONS

int rssiToQuality(int rssi) {
  int quality = 0;
  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

} // namespace WiFiManagerUtils

