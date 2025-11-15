/**
 * HTML.h
 * HTML template fragments for WiFiManager web interface
 * Static PROGMEM strings to reduce string concatenations
 */

#ifndef _HTML_TEMPLATES_H_
#define _HTML_TEMPLATES_H_

#include <Arduino.h>

// HTML document structure
const char HTML_HEAD_START[] PROGMEM = 
"<!DOCTYPE html>"
"<html lang='en'><head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta  name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
"<title>";

const char HTML_TITLE_END[] PROGMEM = "</title>";

const char HTML_HEAD_END_START[] PROGMEM = "</head><body class='";

const char HTML_HEAD_END_WRAP[] PROGMEM = "'><div class='wrap'>";

const char HTML_END[] PROGMEM = "</div></body></html>";

// Form elements
const char HTML_FORM_END[] PROGMEM = "<br/><br/><button type='submit'>Save</button></form>";

const char HTML_FORM_STATIC_HEAD[] PROGMEM = "<hr><br/>";

const char HTML_FORM_PARAM_HEAD[] PROGMEM = "<hr><br/>";

const char HTML_FORM_WIFI_END[] PROGMEM = "";

const char HTML_BR[] PROGMEM = "<br/>";

// Buttons
const char HTML_BACKBTN[] PROGMEM = "<hr><br/><form action='/' method='get'><button>Back</button></form>";

const char HTML_ERASEBTN[] PROGMEM = "<br/><form action='/erase' method='get'><button class='D'>Erase WiFi config</button></form>";

// Status messages
const char HTML_STATUS_OFFPW[] PROGMEM = "<br/>Authentication failure";

const char HTML_STATUS_OFFNOAP[] PROGMEM = "<br/>AP not found";

const char HTML_STATUS_OFFFAIL[] PROGMEM = "<br/>Could not connect";

const char HTML_STATUS_NONE[] PROGMEM = "<div class='msg'>No AP set</div>";

const char HTML_SAVED[] PROGMEM = "<div class='msg'>Saving Credentials<br/>Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";

const char HTML_PARAMSAVED[] PROGMEM = "<div class='msg S'>Saved<br/></div>";

// Update page
const char HTML_UPDATE[] PROGMEM = "Upload new firmware<br/><form method='POST' action='u' enctype='multipart/form-data' onchange=\"(function(el){document.getElementById('uploadbin').style.display = el.value=='' ? 'none' : 'initial';})(this)\"><input type='file' name='update' accept='.bin,application/octet-stream'><button id='uploadbin' type='submit' class='h D'>Update</button></form><small><a href='http://192.168.4.1/update' target='_blank'>* May not function inside captive portal, open in browser http://192.168.4.1</a></small>";

const char HTML_UPDATE_FAIL[] PROGMEM = "<div class='msg D'><strong>Update failed!</strong><Br/>Reboot device and try again</div>";

const char HTML_UPDATE_SUCCESS[] PROGMEM = "<div class='msg S'><strong>Update successful.  </strong> <br/> Device rebooting now...</div>";

// Portal menu
const char * const HTML_PORTAL_MENU[] PROGMEM = {
"<form action='/wifi'    method='get'><button>Configure WiFi</button></form><br/>\n", // MENU_WIFI
"<form action='/0wifi'   method='get'><button>Configure WiFi (No scan)</button></form><br/>\n", // MENU_WIFINOSCAN
"<form action='/info'    method='get'><button>Info</button></form><br/>\n", // MENU_INFO
"<form action='/param'   method='get'><button>Setup</button></form><br/>\n",//MENU_PARAM
"<form action='/close'   method='get'><button>Close</button></form><br/>\n", // MENU_CLOSE
"<form action='/restart' method='get'><button>Restart</button></form><br/>\n",// MENU_RESTART
"<form action='/exit'    method='get'><button>Exit</button></form><br/>\n",  // MENU_EXIT
"<form action='/erase'   method='get'><button class='D'>Erase</button></form><br/>\n", // MENU_ERASE
"<form action='/update'  method='get'><button>Update</button></form><br/>\n",// MENU_UPDATE
"<hr><br/>" // MENU_SEP
};

const char HTML_PORTAL_OPTIONS[] PROGMEM = "";

// Help page
#ifndef WM_NOHELP
const char HTML_HELP[] PROGMEM =
 "<br/><h3>Available pages</h3><hr>"
 "<table class='table'>"
 "<thead><tr><th>Page</th><th>Function</th></tr></thead><tbody>"
 "<tr><td><a href='/'>/</a></td>"
 "<td>Menu page.</td></tr>"
 "<tr><td><a href='/wifi'>/wifi</a></td>"
 "<td>Show WiFi scan results and enter WiFi configuration.(/0wifi noscan)</td></tr>"
 "<tr><td><a href='/wifisave'>/wifisave</a></td>"
 "<td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr>"
 "<tr><td><a href='/param'>/param</a></td>"
 "<td>Parameter page</td></tr>"
 "<tr><td><a href='/info'>/info</a></td>"
 "<td>Information page</td></tr>"
 "<tr><td><a href='/u'>/u</a></td>"
 "<td>OTA Update</td></tr>"
 "<tr><td><a href='/close'>/close</a></td>"
 "<td>Close the captiveportal popup, config portal will remain active</td></tr>"
 "<tr><td>/exit</td>"
 "<td>Exit Config portal, config portal will close</td></tr>"
 "<tr><td>/restart</td>"
 "<td>Reboot the device</td></tr>"
 "<tr><td>/erase</td>"
 "<td>Erase WiFi configuration and reboot device. Device will not reconnect to a network until new WiFi configuration data is entered.</td></tr>"
 "</table>"
 "<p/>Github <a href='https://github.com/tzapu/WiFiManager'>https://github.com/tzapu/WiFiManager</a>.";
#else
const char HTML_HELP[] PROGMEM = "";
#endif

#ifdef WM_JSTEST
const char HTML_JS[] PROGMEM =
"<script>function postAjax(url, data, success) {"
"    var params = typeof data == 'string' ? data : Object.keys(data).map("
"            function(k){ return encodeURIComponent(k) + '=' + encodeURIComponent(data[k]) }"
"        ).join('&');"
"    var xhr = window.XMLHttpRequest ? new XMLHttpRequest() : new ActiveXObject(\"Microsoft.XMLHTTP\");"
"    xhr.open('POST', url);"
"    xhr.onreadystatechange = function() {"
"        if (xhr.readyState>3 && xhr.status==200) { success(xhr.responseText); }"
"    };"
"    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');"
"    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');"
"    xhr.send(params);"
"    return xhr;}"
"postAjax('/status', 'p1=1&p2=Hello+World', function(data){ console.log(data); });"
"postAjax('/status', { p1: 1, p2: 'Hello World' }, function(data){ console.log(data); });"
"</script>";
#endif

#endif // _HTML_TEMPLATES_H_

