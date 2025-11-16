/**
 * Root.h
 * Whole-document root template for WiFiManager using DFTE placeholders
 */

#ifndef _WM_ROOT_TEMPLATE_H_
#define _WM_ROOT_TEMPLATE_H_

#include <Arduino.h>

// Whole document template:
// - Injects %SCRIPTS% and %STYLES% (these already include <script>/<style> tags)
// - Mirrors existing structure: body class "home" and wrapper div.wrap
// - Renders title/subtitle, then menu and status
const char WM_ROOT_TEMPLATE[] PROGMEM =
"<!DOCTYPE html>"
"<html lang='en'>"
"<head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
"<title>%PAGE_TITLE%</title>"
"%SCRIPTS%"
"%STYLES%"
"</head>"
"<body class='home'>"
"<div class='wrap'>"
"<h1>%PAGE_TITLE%</h1>"
"<h3>%SUBTITLE%</h3>"
"%MENU%"
"%STATUS%"
"</div>"
"</body>"
"</html>";

#endif // _WM_ROOT_TEMPLATE_H_


