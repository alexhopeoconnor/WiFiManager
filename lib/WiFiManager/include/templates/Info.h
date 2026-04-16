/**
 * Info.h
 * Streamed info page template for WiFiManager.
 */

#ifndef _WM_INFO_TEMPLATE_H_
#define _WM_INFO_TEMPLATE_H_

#include <Arduino.h>

const char WM_INFO_TEMPLATE[] PROGMEM =
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
"<body class='info'>"
"<div class='wrap'>"
"%INFO_STATUS%"
"%INFO_DEVICE_SECTION%"
"%INFO_WIFI_SECTION%"
"%INFO_ABOUT_SECTION%"
"</div>"
"</body>"
"</html>";

#endif // _WM_INFO_TEMPLATE_H_
