/**
 * Info.h
 * Streamed info page content template for WiFiManager.
 */

#ifndef _WM_INFO_TEMPLATE_H_
#define _WM_INFO_TEMPLATE_H_

#include <Arduino.h>

const char WM_INFO_CONTENT_TEMPLATE[] PROGMEM =
"%INFO_STATUS%"
"<section class='info-device'>%INFO_DEVICE_SECTION%</section>"
"<section class='info-wifi'>%INFO_WIFI_SECTION%</section>"
"<section class='info-about'>%INFO_ABOUT_SECTION%</section>"
"%INFO_FOOTER%";

#endif // _WM_INFO_TEMPLATE_H_
