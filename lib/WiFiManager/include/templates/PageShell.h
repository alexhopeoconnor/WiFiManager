/**
 * PageShell.h
 * Generic whole-document shell template for streamed WiFiManager pages.
 */
 
#ifndef _WM_PAGE_SHELL_TEMPLATE_H_
#define _WM_PAGE_SHELL_TEMPLATE_H_

#include <Arduino.h>

const char WM_PAGE_SHELL_TEMPLATE[] PROGMEM =
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
"<body class='%BODY_CLASS%'>"
"<div class='wrap'>"
"%PAGE_CONTENT%"
"</div>"
"</body>"
"</html>";

#endif // _WM_PAGE_SHELL_TEMPLATE_H_
