/**
 * HTML.h
 * HTML template fragments for WiFiManager web interface
 * Static PROGMEM strings to reduce string concatenations
 */

#ifndef _HTML_TEMPLATES_H_
#define _HTML_TEMPLATES_H_

#include <Arduino.h>

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

// Help page
#ifndef WM_NOHELP
const char HTML_HELP[] PROGMEM =
 "<br/><h3>Available pages</h3><hr>"
 "<table class='table'>"
 "<thead><tr><th>Page</th><th>Function</th></tr></thead><tbody>"
 "<tr><td><a href='/'>/</a></td>"
 "<td>Menu page.</td></tr>"
 "<tr><td><a href='/wifi'>/wifi</a></td>"
 "<td>Show WiFi scan results and enter WiFi configuration.</td></tr>"
 "<tr><td><a href='/0wifi'>/0wifi</a></td>"
 "<td>Show WiFi configuration without loading scan results until refresh is requested.</td></tr>"
 "<tr><td><a href='/wifisave'>/wifisave</a></td>"
 "<td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr>"
 "<tr><td><a href='/param'>/param</a></td>"
 "<td>Parameter page</td></tr>"
 "<tr><td><a href='/info'>/info</a></td>"
 "<td>Information page</td></tr>"
 "<tr><td><a href='/update'>/update</a></td>"
 "<td>OTA update page. Firmware upload posts to /u.</td></tr>"
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

#endif // _HTML_TEMPLATES_H_

