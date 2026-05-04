/**
 * RootShell.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * Single-page portal shell: only HTML document served for GET /.
 * Dynamic UI is driven by embedded JS + JSON APIs under /api/...
 *
 * Placeholder keys (%PAGE_TITLE%, %STYLES%, %BOOTSTRAP_JSON%, %PORTAL_APP_JS%, %PORTAL_APPEND_JS%) are filled
 * per request in WiFiManagerHandlers::handleRoot; do not treat placeholders as a customization API.
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
    "%STYLES%"
    "</head>"
    "<body class='portal'>"
    "<div id='wm-toast' class='wm-toast' aria-live='polite' role='status' style='display:none'></div>"
    "<div id='wm-dialog' class='wm-dialog' style='display:none' aria-hidden='true'>"
    "<div class='wm-dialog-backdrop' id='wm-dialog-backdrop'></div>"
    "<div class='wm-dialog-panel'>"
    "<p class='wm-dialog-msg' id='wm-dialog-msg'></p>"
    "<div class='wm-dialog-actions'>"
    "<button type='button' id='wm-dialog-cancel' class='wm-btn wm-btn--secondary'>Cancel</button>"
    "<button type='button' id='wm-dialog-ok' class='wm-btn wm-btn--primary wm-dialog-ok'>OK</button>"
    "</div></div></div>"
    "<div id='app'></div>"
    "<script id='wm-bootstrap' type='application/json'>%BOOTSTRAP_JSON%</script>"
    "<script>"
    "%PORTAL_APP_JS%"
    "</script>"
    "<script>"
    "%PORTAL_APPEND_JS%"
    "</script>"
    "</body>"
    "</html>";

#endif  // _WM_ROOT_SHELL_TEMPLATE_H_
