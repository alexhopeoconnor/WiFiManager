/**
 * Param.h
 * Streamed setup page content template for WiFiManager.
 */

#ifndef _WM_PARAM_TEMPLATE_H_
#define _WM_PARAM_TEMPLATE_H_

#include <Arduino.h>

const char WM_PARAM_CONTENT_TEMPLATE[] PROGMEM =
"<form method='POST' action='paramsave'>"
"%PARAM_FIELDS%"
"%PARAM_FORM_ACTIONS%</form>"
"%PARAM_PAGE_ACTIONS%"
"%PARAM_STATUS%";

#endif // _WM_PARAM_TEMPLATE_H_
