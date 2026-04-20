/**
 * RootShell.h
 * Single-page portal shell: only HTML document served for GET /.
 * Dynamic UI is driven by embedded JS + JSON APIs under /api/*.
 */

#ifndef _WM_ROOT_SHELL_TEMPLATE_H_
#define _WM_ROOT_SHELL_TEMPLATE_H_

#include <Arduino.h>

const char WM_ROOT_SHELL_TEMPLATE[] PROGMEM =
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta name='format-detection' content='telephone=no'>"
    "<meta charset='UTF-8'/>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
    "<title>%PAGE_TITLE%</title>"
    "%SCRIPTS%"
    "%STYLES%"
    "</head>"
    "<body class='portal'>"
    "<div id='app'></div>"
    "<script id='wm-bootstrap' type='application/json'>%BOOTSTRAP_JSON%</script>"
    "<script>"
    "%PORTAL_APP_JS%"
    "</script>"
    "</body>"
    "</html>";

#endif  // _WM_ROOT_SHELL_TEMPLATE_H_
