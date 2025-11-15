/**
 * WiFiManagerUtils.h
 * Pure utility functions for WiFiManager
 * String conversions, HTML encoding, and helper calculations
 * These functions have no dependency on WiFiManager instance state
 *
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#ifndef _WIFI_MANAGER_UTILS_H_
#define _WIFI_MANAGER_UTILS_H_

#include <Arduino.h>

// Forward declaration
class IPAddress;

namespace WiFiManagerUtils {

// -----------------------------------------------------------------------------------------------
// WIFI STATUS ARRAYS (for status string conversion)

extern const char * const WIFI_STA_STATUS[] PROGMEM;
extern const char * const WIFI_MODES[] PROGMEM;

#ifdef ESP32
extern const char * const AUTH_MODE_NAMES[] PROGMEM;
#elif defined(ESP8266)
extern const char * const AUTH_MODE_NAMES[] PROGMEM;
#endif

// -----------------------------------------------------------------------------------------------
// STRING CONVERSION UTILITIES

/**
 * Convert WiFi status code to readable string
 * @param status WiFi status code
 * @return Status string or "Unknown" if invalid
 */
String getStatusString(uint8_t status);


/**
 * Convert WiFi mode code to readable string
 * @param mode WiFi mode code
 * @return Mode string or "Unknown" if invalid
 */
String getModeString(uint8_t mode);

/**
 * Convert encryption/auth mode to readable string
 * @param authmode Encryption type code
 * @return Encryption type string
 */
String getEncryptionString(uint8_t authmode);

// -----------------------------------------------------------------------------------------------
// HTML UTILITIES

/**
 * Encode HTML entities in a string
 * @param str String to encode
 * @param whitespace If true, also encode spaces as &#160;
 * @return Encoded string
 */
String htmlEntities(const String& str, bool whitespace = false);

// -----------------------------------------------------------------------------------------------
// IP/STRING UTILITIES

/**
 * Check if string is a valid IP address
 * @param str String to check
 * @return true if valid IP address
 */
bool isValidIP(const String& str);

/**
 * Convert IPAddress to String
 * @param ip IPAddress to convert
 * @return IP address string
 */
String ipToString(IPAddress ip);

/**
 * Validate AP password format (8-63 characters)
 * @param password Password to validate
 * @return true if valid
 */
bool isValidAPPassword(const String& password);

// -----------------------------------------------------------------------------------------------
// HELPER CALCULATIONS

/**
 * Convert RSSI value to quality percentage (0-100)
 * @param rssi RSSI value in dBm
 * @return Quality percentage (0-100)
 */
int rssiToQuality(int rssi);

} // namespace WiFiManagerUtils

#endif // _WIFI_MANAGER_UTILS_H_

