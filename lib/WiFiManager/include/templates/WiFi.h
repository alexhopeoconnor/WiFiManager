/**
 * WiFi.h
 * Streamed WiFi configuration page content template for WiFiManager.
 */

#ifndef _WM_WIFI_TEMPLATE_H_
#define _WM_WIFI_TEMPLATE_H_

#include <Arduino.h>

const char WM_WIFI_CONTENT_TEMPLATE[] PROGMEM =
"<div id='scan-results'>%WIFI_SCAN_CONTENT%</div>"
"<form method='POST' action='wifisave'>"
"<label for='s'>SSID</label>"
"<input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='%WIFI_SSID_PLACEHOLDER%'>"
"<br/>"
"<label for='p'>Password</label>"
"<input id='p' name='p' maxlength='64' type='password' placeholder='%WIFI_PASSWORD_PLACEHOLDER%'>"
"<input type='checkbox' id='showpass' onclick='f()'> <label for='showpass'>Show Password</label><br/>"
"%WIFI_STATIC_FIELDS%"
"%WIFI_PARAM_SECTION%"
"%WIFI_FORM_ACTIONS%</form>"
"%WIFI_PAGE_ACTIONS%"
"%WIFI_STATUS%";

#endif // _WM_WIFI_TEMPLATE_H_
