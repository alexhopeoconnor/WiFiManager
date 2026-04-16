/**
 * WiFi.h
 * Streamed WiFi configuration page template for WiFiManager.
 */

#ifndef _WM_WIFI_TEMPLATE_H_
#define _WM_WIFI_TEMPLATE_H_

#include <Arduino.h>

const char WM_WIFI_TEMPLATE[] PROGMEM =
"<!DOCTYPE html>"
"<html lang='en'>"
"<head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
"<title>%DOC_TITLE%</title>"
"%SCRIPTS%"
"%STYLES%"
"</head>"
"<body class='wifi'>"
"<div class='wrap'>"
"%WIFI_SCAN_RESULTS%"
"%WIFI_FORM_SECTION%"
"%WIFI_BACK_SECTION%"
"%WIFI_STATUS%"
"</div>"
"</body>"
"</html>";

#endif // _WM_WIFI_TEMPLATE_H_
