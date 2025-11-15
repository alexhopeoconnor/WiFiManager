/**
 * WiFiManagerHandlers.cpp
 * 
 * HTTP request handlers and rendering logic implementation
 * 
 * @author tablatronix
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#include "WiFiManagerHandlers.h"
#include "WiFiManager.h"

#if defined(ESP8266) || defined(ESP32)

WiFiManagerHandlers::WiFiManagerHandlers(WiFiManager* wm) : _wm(wm) {
}

// Rendering Methods

String WiFiManagerHandlers::getHTTPHead(String title, String classes){
  String page;
  page += FPSTR(HTTP_HEAD_START);
  page.replace(FPSTR(T_v), title);
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _wm->_customHeadElement;

  String p = FPSTR(HTTP_HEAD_END);
  if (_wm->_bodyClass != "") {
    if (classes != "") {
      classes += " ";  // add spacing, if necessary
    }
    classes += _wm->_bodyClass;  // add class str
  }
  p.replace(FPSTR(T_c), classes);
  page += p;

  if (_wm->_customBodyHeader) {
    page += _wm->_customBodyHeader;
  }

  return page;
}

String WiFiManagerHandlers::getHTTPEnd() {
  String end = FPSTR(HTTP_END);

  if (_wm->_customBodyFooter) {
    end = String(_wm->_customBodyFooter) + end;
  }

  return end;
}

String WiFiManagerHandlers::getMenuOut(){
  String page;  

  for(auto menuId : _wm->_menuIds ){
    if((String)_menutokens[menuId] == "param" && _wm->_paramsCount == 0) continue; // no params set, omit params from menu, @todo this may be undesired by someone, use only menu to force?
    if((String)_menutokens[menuId] == "custom" && _wm->_customMenuHTML!=NULL){
      page += _wm->_customMenuHTML;
      continue;
    }
    page += HTTP_PORTAL_MENU[menuId];
    delay(0);
  }

  return page;
}

String WiFiManagerHandlers::getScanItemOut(){
    String page;

    // Never trigger scans from here - only use cached data
    // If no cached data, show message (scan should be started elsewhere if needed)
    int n = _wm->_numNetworks;
    if (n == 0) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(F("No networks found"));
      #endif
      if(_wm->_scanInProgress){
        page += F("Scanning for networks...<br/><br/>");
      } else {
        page += FPSTR(S_nonetworks); // @token nonetworks
        page += F("<br/><br/>");
      }
    }
    else {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(n,F("networks found"));
      #endif
      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }

      // RSSI SORT
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      // remove duplicates ( must be RSSI sorted )
      if (_wm->_removeDuplicateAPs) {
        String cssid;
        for (int i = 0; i < n; i++) {
          if (indices[i] == -1) continue;
          cssid = WiFi.SSID(indices[i]);
          for (int j = i + 1; j < n; j++) {
            if (cssid == WiFi.SSID(indices[j])) {
              #ifdef WM_DEBUG_LEVEL
              _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("DUP AP:"),WiFi.SSID(indices[j]));
              #endif
              indices[j] = -1; // set dup aps to index -1
            }
          }
        }
      }

      // token precheck, to speed up replacements on large ap lists
      String HTTP_ITEM_STR = FPSTR(HTTP_ITEM);

      // toggle icons with percentage
      HTTP_ITEM_STR.replace("{qp}", FPSTR(HTTP_ITEM_QP));
      HTTP_ITEM_STR.replace("{h}",_wm->_scanDispOptions ? "" : "h");
      HTTP_ITEM_STR.replace("{qi}", FPSTR(HTTP_ITEM_QI));
      HTTP_ITEM_STR.replace("{h}",_wm->_scanDispOptions ? "h" : "");
 
      // set token precheck flags
      bool tok_r = HTTP_ITEM_STR.indexOf(FPSTR(T_r)) > 0;
      bool tok_R = HTTP_ITEM_STR.indexOf(FPSTR(T_R)) > 0;
      bool tok_e = HTTP_ITEM_STR.indexOf(FPSTR(T_e)) > 0;
      bool tok_q = HTTP_ITEM_STR.indexOf(FPSTR(T_q)) > 0;
      bool tok_i = HTTP_ITEM_STR.indexOf(FPSTR(T_i)) > 0;
      
      //display networks in page
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups

        #ifdef WM_DEBUG_LEVEL
        _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("AP: "),(String)WiFi.RSSI(indices[i]) + " " + (String)WiFi.SSID(indices[i]));
        #endif

        int rssiperc = _wm->getRSSIasQuality(WiFi.RSSI(indices[i]));
        uint8_t enc_type = WiFi.encryptionType(indices[i]);

        if (_wm->_minimumQuality == -1 || _wm->_minimumQuality < rssiperc) {
          String item = HTTP_ITEM_STR;
          if(WiFi.SSID(indices[i]) == ""){
            continue; // No idea why I am seeing these, lets just skip them for now
          }
          item.replace(FPSTR(T_V), _wm->htmlEntities(WiFi.SSID(indices[i]))); // ssid no encoding
          item.replace(FPSTR(T_v), _wm->htmlEntities(WiFi.SSID(indices[i]),true)); // ssid no encoding
          if(tok_e) item.replace(FPSTR(T_e), _wm->encryptionTypeStr(enc_type));
          if(tok_r) item.replace(FPSTR(T_r), (String)rssiperc); // rssi percentage 0-100
          if(tok_R) item.replace(FPSTR(T_R), (String)WiFi.RSSI(indices[i])); // rssi db
          if(tok_q) item.replace(FPSTR(T_q), (String)int(round(map(rssiperc,0,100,1,4)))); //quality icon 1-4
          if(tok_i){
            if (enc_type != WM_WIFIOPEN) {
              item.replace(FPSTR(T_i), F("l"));
            } else {
              item.replace(FPSTR(T_i), "");
            }
          }
          #ifdef WM_DEBUG_LEVEL
          _wm->DEBUG_WM(WM_DEBUG_DEV,item);
          #endif
          page += item;
          delay(0);
        } else {
          #ifdef WM_DEBUG_LEVEL
          _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("Skipping , does not meet _minimumQuality"));
          #endif
        }

      }
      page += FPSTR(HTTP_BR);
    }

    return page;
}

String WiFiManagerHandlers::getIpForm(String id, String title, String value){
    String item = FPSTR(HTTP_FORM_LABEL);
    item += FPSTR(HTTP_FORM_PARAM);
    item.replace(FPSTR(T_i), id);
    item.replace(FPSTR(T_n), id);
    item.replace(FPSTR(T_p), FPSTR(T_t));
    item.replace(FPSTR(T_t), title);
    item.replace(FPSTR(T_l), F("15"));
    item.replace(FPSTR(T_v), value);
    item.replace(FPSTR(T_c), "");
    return item;  
}

String WiFiManagerHandlers::getStaticOut(){
  String page;
  if ((_wm->_staShowStaticFields || _wm->_sta_static_ip) && _wm->_staShowStaticFields>=0) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV,F("_staShowStaticFields"));
    #endif
    page += FPSTR(HTTP_FORM_STATIC_HEAD);
    page += getIpForm(FPSTR(S_ip),FPSTR(S_staticip),(_wm->_sta_static_ip ? _wm->_sta_static_ip.toString() : ""));
    page += getIpForm(FPSTR(S_gw),FPSTR(S_staticgw),(_wm->_sta_static_gw ? _wm->_sta_static_gw.toString() : ""));
    page += getIpForm(FPSTR(S_sn),FPSTR(S_subnet),(_wm->_sta_static_sn ? _wm->_sta_static_sn.toString() : ""));
  }

  if((_wm->_staShowDns || _wm->_sta_static_dns) && _wm->_staShowDns>=0){
    page += getIpForm(FPSTR(S_dns),FPSTR(S_staticdns),(_wm->_sta_static_dns ? _wm->_sta_static_dns.toString() : ""));
  }

  if(page!="") page += FPSTR(HTTP_BR);

  return page;
}

String WiFiManagerHandlers::getParamOut(){
  String page;

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("getParamOut"),_wm->_paramsCount);
  #endif

  if(_wm->_paramsCount > 0){

    String HTTP_PARAM_temp = FPSTR(HTTP_FORM_LABEL);
    HTTP_PARAM_temp += FPSTR(HTTP_FORM_PARAM);
    bool tok_I = HTTP_PARAM_temp.indexOf(FPSTR(T_I)) > 0;
    bool tok_i = HTTP_PARAM_temp.indexOf(FPSTR(T_i)) > 0;
    bool tok_n = HTTP_PARAM_temp.indexOf(FPSTR(T_n)) > 0;
    bool tok_p = HTTP_PARAM_temp.indexOf(FPSTR(T_p)) > 0;
    bool tok_t = HTTP_PARAM_temp.indexOf(FPSTR(T_t)) > 0;
    bool tok_l = HTTP_PARAM_temp.indexOf(FPSTR(T_l)) > 0;
    bool tok_v = HTTP_PARAM_temp.indexOf(FPSTR(T_v)) > 0;
    bool tok_c = HTTP_PARAM_temp.indexOf(FPSTR(T_c)) > 0;

    char valLength[5];

    for (int i = 0; i < _wm->_paramsCount; i++) {
      if (_wm->_params[i] == NULL || _wm->_params[i]->getValueLength() > 99999) {
        #ifdef WM_DEBUG_LEVEL
        _wm->DEBUG_WM(WM_DEBUG_ERROR,F("[ERROR] WiFiManagerParameter is out of scope"));
        #endif
        return "";
      }
    }

    // add the extra parameters to the form
    for (int i = 0; i < _wm->_paramsCount; i++) {
     String pitem;
      switch (_wm->_params[i]->getLabelPlacement()) {
        case WFM_LABEL_BEFORE:
          pitem = FPSTR(HTTP_FORM_LABEL);
          pitem += FPSTR(HTTP_FORM_PARAM);
          break;
        case WFM_LABEL_AFTER:
          pitem = FPSTR(HTTP_FORM_PARAM);
          pitem += FPSTR(HTTP_FORM_LABEL);
          break;
        default:
          pitem = FPSTR(HTTP_FORM_PARAM);
          break;
      }

      if (_wm->_params[i]->getID() != NULL) {
        if(tok_I)pitem.replace(FPSTR(T_I), (String)FPSTR(S_parampre)+(String)i);
        if(tok_i)pitem.replace(FPSTR(T_i), _wm->_params[i]->getID());
        if(tok_n)pitem.replace(FPSTR(T_n), _wm->_params[i]->getID());
        if(tok_p)pitem.replace(FPSTR(T_p), FPSTR(T_t));
        if(tok_t)pitem.replace(FPSTR(T_t), _wm->_params[i]->getLabel());
        snprintf(valLength, 5, "%d", _wm->_params[i]->getValueLength());
        if(tok_l)pitem.replace(FPSTR(T_l), valLength);
        if(tok_v)pitem.replace(FPSTR(T_v), _wm->_params[i]->getValue());
        if(tok_c)pitem.replace(FPSTR(T_c), _wm->_params[i]->getCustomHTML());
      } else {
        pitem = _wm->_params[i]->getCustomHTML();
      }

      page += pitem;
    }
  }

  return page;
}

String WiFiManagerHandlers::getInfoData(String id){
  String p;
  if(id==F("esphead")){
    p = FPSTR(HTTP_INFO_esphead);
    #ifdef ESP32
      p.replace(FPSTR(T_1), (String)ESP.getChipModel());
    #endif
  }
  else if(id==F("wifihead")){
    p = FPSTR(HTTP_INFO_wifihead);
    p.replace(FPSTR(T_1),_wm->getModeString(WiFi.getMode()));
  }
  else if(id==F("uptime")){
    p = FPSTR(HTTP_INFO_uptime);
    p.replace(FPSTR(T_1),(String)(millis() / 1000 / 60));
    p.replace(FPSTR(T_2),(String)((millis() / 1000) % 60));
  }
  else if(id==F("chipid")){
    p = FPSTR(HTTP_INFO_chipid);
    #ifdef ESP8266
      p.replace(FPSTR(T_1),String(ESP.getChipId(),HEX));
    #elif defined(ESP32)
      p.replace(FPSTR(T_1),String((uint32_t)ESP.getEfuseMac(),HEX));
    #endif
  }
  #ifdef ESP32
  else if(id==F("chiprev")){
      p = FPSTR(HTTP_INFO_chiprev);
      String rev = (String)ESP.getChipRevision();
      #ifdef _SOC_EFUSE_REG_H_
        String revb = (String)(REG_READ(EFUSE_BLK0_RDATA3_REG) >> (EFUSE_RD_CHIP_VER_RESERVE_S)&&EFUSE_RD_CHIP_VER_RESERVE_V);
        p.replace(FPSTR(T_1),rev+"<br/>"+revb);
      #else
        p.replace(FPSTR(T_1),rev);
      #endif
  }
  #endif
  #ifdef ESP8266
  else if(id==F("fchipid")){
      p = FPSTR(HTTP_INFO_fchipid);
      p.replace(FPSTR(T_1),(String)ESP.getFlashChipId());
  }
  #endif
  else if(id==F("idesize")){
    p = FPSTR(HTTP_INFO_idesize);
    p.replace(FPSTR(T_1),(String)ESP.getFlashChipSize());
  }
  else if(id==F("flashsize")){
    #ifdef ESP8266
      p = FPSTR(HTTP_INFO_flashsize);
      p.replace(FPSTR(T_1),(String)ESP.getFlashChipRealSize());
    #elif defined ESP32
      p = FPSTR(HTTP_INFO_psrsize);
      p.replace(FPSTR(T_1),(String)ESP.getPsramSize());      
    #endif
  }
  else if(id==F("corever")){
    #ifdef ESP8266
      p = FPSTR(HTTP_INFO_corever);
      p.replace(FPSTR(T_1),(String)ESP.getCoreVersion());
    #endif      
  }
  #ifdef ESP8266
  else if(id==F("bootver")){
      p = FPSTR(HTTP_INFO_bootver);
      p.replace(FPSTR(T_1),(String)system_get_boot_version());
  }
  #endif
  else if(id==F("cpufreq")){
    p = FPSTR(HTTP_INFO_cpufreq);
    p.replace(FPSTR(T_1),(String)ESP.getCpuFreqMHz());
  }
  else if(id==F("freeheap")){
    p = FPSTR(HTTP_INFO_freeheap);
    p.replace(FPSTR(T_1),(String)ESP.getFreeHeap());
  }
  else if(id==F("memsketch")){
    p = FPSTR(HTTP_INFO_memsketch);
    p.replace(FPSTR(T_1),(String)(ESP.getSketchSize()));
    p.replace(FPSTR(T_2),(String)(ESP.getSketchSize()+ESP.getFreeSketchSpace()));
  }
  else if(id==F("memsmeter")){
    p = FPSTR(HTTP_INFO_memsmeter);
    p.replace(FPSTR(T_1),(String)(ESP.getSketchSize()));
    p.replace(FPSTR(T_2),(String)(ESP.getSketchSize()+ESP.getFreeSketchSpace()));
  }
  else if(id==F("lastreset")){
    #ifdef ESP8266
      p = FPSTR(HTTP_INFO_lastreset);
      p.replace(FPSTR(T_1),(String)ESP.getResetReason());
    #elif defined(ESP32) && defined(_ROM_RTC_H_)
      p = FPSTR(HTTP_INFO_lastreset);
      for(int i=0;i<2;i++){
        int reason = rtc_get_reset_reason(i);
        String tok = (String)T_ss+(String)(i+1)+(String)T_es;
        switch (reason)
        {
          case 1  : p.replace(tok,F("Vbat power on reset"));break;
          case 3  : p.replace(tok,F("Software reset digital core"));break;
          case 4  : p.replace(tok,F("Legacy watch dog reset digital core"));break;
          case 5  : p.replace(tok,F("Deep Sleep reset digital core"));break;
          case 6  : p.replace(tok,F("Reset by SLC module, reset digital core"));break;
          case 7  : p.replace(tok,F("Timer Group0 Watch dog reset digital core"));break;
          case 8  : p.replace(tok,F("Timer Group1 Watch dog reset digital core"));break;
          case 9  : p.replace(tok,F("RTC Watch dog Reset digital core"));break;
          case 10 : p.replace(tok,F("Instrusion tested to reset CPU"));break;
          case 11 : p.replace(tok,F("Time Group reset CPU"));break;
          case 12 : p.replace(tok,F("Software reset CPU"));break;
          case 13 : p.replace(tok,F("RTC Watch dog Reset CPU"));break;
          case 14 : p.replace(tok,F("for APP CPU, reseted by PRO CPU"));break;
          case 15 : p.replace(tok,F("Reset when the vdd voltage is not stable"));break;
          case 16 : p.replace(tok,F("RTC Watch dog reset digital core and rtc module"));break;
          default : p.replace(tok,F("NO_MEAN"));
        }
      }
    #endif
  }
  else if(id==F("apip")){
    p = FPSTR(HTTP_INFO_apip);
    p.replace(FPSTR(T_1),WiFi.softAPIP().toString());
  }
  else if(id==F("apmac")){
    p = FPSTR(HTTP_INFO_apmac);
    p.replace(FPSTR(T_1),(String)WiFi.softAPmacAddress());
  }
  #ifdef ESP32
  else if(id==F("aphost")){
      p = FPSTR(HTTP_INFO_aphost);
      p.replace(FPSTR(T_1),WiFi.softAPgetHostname());
  }
  #endif
  #ifndef WM_NOSOFTAPSSID
  #ifdef ESP8266
  else if(id==F("apssid")){
    p = FPSTR(HTTP_INFO_apssid);
    p.replace(FPSTR(T_1),_wm->htmlEntities(WiFi.softAPSSID()));
  }
  #endif
  #endif
  else if(id==F("apbssid")){
    p = FPSTR(HTTP_INFO_apbssid);
    p.replace(FPSTR(T_1),(String)WiFi.BSSIDstr());
  }
  else if(id==F("stassid")){
    p = FPSTR(HTTP_INFO_stassid);
    p.replace(FPSTR(T_1),_wm->htmlEntities((String)_wm->WiFi_SSID()));
  }
  else if(id==F("staip")){
    p = FPSTR(HTTP_INFO_staip);
    p.replace(FPSTR(T_1),WiFi.localIP().toString());
  }
  else if(id==F("stagw")){
    p = FPSTR(HTTP_INFO_stagw);
    p.replace(FPSTR(T_1),WiFi.gatewayIP().toString());
  }
  else if(id==F("stasub")){
    p = FPSTR(HTTP_INFO_stasub);
    p.replace(FPSTR(T_1),WiFi.subnetMask().toString());
  }
  else if(id==F("dnss")){
    p = FPSTR(HTTP_INFO_dnss);
    p.replace(FPSTR(T_1),WiFi.dnsIP().toString());
  }
  else if(id==F("host")){
    p = FPSTR(HTTP_INFO_host);
    #ifdef ESP32
      p.replace(FPSTR(T_1),WiFi.getHostname());
    #else
    p.replace(FPSTR(T_1),WiFi.hostname());
    #endif
  }
  else if(id==F("stamac")){
    p = FPSTR(HTTP_INFO_stamac);
    p.replace(FPSTR(T_1),WiFi.macAddress());
  }
  else if(id==F("conx")){
    p = FPSTR(HTTP_INFO_conx);
    p.replace(FPSTR(T_1),WiFi.isConnected() ? FPSTR(S_y) : FPSTR(S_n));
  }
  #ifdef ESP8266
  else if(id==F("autoconx")){
    p = FPSTR(HTTP_INFO_autoconx);
    p.replace(FPSTR(T_1),WiFi.getAutoConnect() ? FPSTR(S_enable) : FPSTR(S_disable));
  }
  #endif
  #if defined(ESP32) && !defined(WM_NOTEMP)
  else if(id==F("temp")){
    p = FPSTR(HTTP_INFO_temp);
    p.replace(FPSTR(T_1),(String)temperatureRead());
    p.replace(FPSTR(T_2),(String)((temperatureRead()+32)*1.8f));
  }
  #endif
  else if(id==F("aboutver")){
    p = FPSTR(HTTP_INFO_aboutver);
    p.replace(FPSTR(T_1),FPSTR(WM_VERSION_STR));
  }
  else if(id==F("aboutarduinover")){
    #ifdef VER_ARDUINO_STR
    p = FPSTR(HTTP_INFO_aboutarduino);
    p.replace(FPSTR(T_1),String(VER_ARDUINO_STR));
    #endif
  }
  else if(id==F("aboutsdkver")){
    p = FPSTR(HTTP_INFO_sdkver);
    #ifdef ESP32
      p.replace(FPSTR(T_1),(String)esp_get_idf_version());
    #else
    p.replace(FPSTR(T_1),(String)system_get_sdk_version());
    #endif
  }
  else if(id==F("aboutdate")){
    p = FPSTR(HTTP_INFO_aboutdate);
    p.replace(FPSTR(T_1),String(__DATE__ " " __TIME__));
  }
  return p;
}

void WiFiManagerHandlers::reportStatus(String &page){
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("[WIFI] reportStatus prev:"),_wm->getWLStatusString(_wm->_lastconxresult));
  _wm->DEBUG_WM(WM_DEBUG_DEV,F("[WIFI] reportStatus current:"),_wm->getWLStatusString(WiFi.status()));
  String str;
  if (_wm->WiFi_SSID() != ""){
    if (WiFi.status()==WL_CONNECTED){
      str = FPSTR(HTTP_STATUS_ON);
      str.replace(FPSTR(T_i),WiFi.localIP().toString());
      str.replace(FPSTR(T_v),_wm->htmlEntities(_wm->WiFi_SSID()));
    }
    else {
      str = FPSTR(HTTP_STATUS_OFF);
      str.replace(FPSTR(T_v),_wm->htmlEntities(_wm->WiFi_SSID()));
      if(_wm->_lastconxresult == _wm->WL_STATION_WRONG_PASSWORD){
        str.replace(FPSTR(T_c),"D");
        str.replace(FPSTR(T_r),FPSTR(HTTP_STATUS_OFFPW));
      }
      else if(_wm->_lastconxresult == WL_NO_SSID_AVAIL){
        str.replace(FPSTR(T_c),"D");
        str.replace(FPSTR(T_r),FPSTR(HTTP_STATUS_OFFNOAP));
      }
      else if(_wm->_lastconxresult == WL_CONNECT_FAILED){
        str.replace(FPSTR(T_c),"D");
        str.replace(FPSTR(T_r),FPSTR(HTTP_STATUS_OFFFAIL));
      }
      else if(_wm->_lastconxresult == WL_CONNECTION_LOST){
        str.replace(FPSTR(T_c),"D");
        str.replace(FPSTR(T_r),FPSTR(HTTP_STATUS_OFFFAIL));
      }
      else{
        str.replace(FPSTR(T_c),"");
        str.replace(FPSTR(T_r),"");
      } 
    }
  }
  else {
    str = FPSTR(HTTP_STATUS_NONE);
  }
  page += str;
}

// Captive Portal

boolean WiFiManagerHandlers::captivePortal(AsyncWebServerRequest *request) {
  
  if(!_wm->_enableCaptivePortal || !_wm->configPortalActive) return false;
  
  String serverLoc = _wm->toStringIp(request->client()->localIP());

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, "-> " + request->host());
  _wm->DEBUG_WM(WM_DEBUG_DEV, "serverLoc " + serverLoc);
  #endif

  // fallback for ipv6 bug
  if(serverLoc == "0.0.0.0"){
    if ((WiFi.status()) != WL_CONNECTED)
      serverLoc = _wm->toStringIp(WiFi.softAPIP());
    else
      serverLoc = _wm->toStringIp(WiFi.localIP());
  }
  
  if(_wm->_httpPort != 80) serverLoc += ":" + (String)_wm->_httpPort;
  bool doredirect = serverLoc != request->host();
  
  if (doredirect) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Request redirected to captive portal"));
    _wm->DEBUG_WM(WM_DEBUG_DEV, "serverLoc " + serverLoc);
    _wm->DEBUG_WM(WM_DEBUG_DEV, "Original URL " + request->url());
    #endif
    String redirectUrl = (String)F("http://") + serverLoc + request->url();
    if (request->params() > 0) {
      redirectUrl += F("?");
      for (size_t i = 0; i < request->params(); i++) {
        if (i > 0) redirectUrl += F("&");
        redirectUrl += request->getParam(i)->name() + F("=") + request->getParam(i)->value();
      }
    }
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, "Redirect URL " + redirectUrl);
    #endif
    request->redirect(redirectUrl);
    return true;
  }
  return false;
}

void WiFiManagerHandlers::stopCaptivePortal(){
  _wm->_enableCaptivePortal = false;
}

// HTTP Handlers

void WiFiManagerHandlers::handleRequest(AsyncWebServerRequest *request) {
  _wm->_webPortalAccessed = millis();
}

void WiFiManagerHandlers::handleRoot(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Root"));
  #endif
  if (captivePortal(request)) return;
  handleRequest(request);
  String page = getHTTPHead(_wm->_title, FPSTR(C_root));
  String str  = FPSTR(HTTP_ROOT_MAIN);
  str.replace(FPSTR(T_t), _wm->_title);
  str.replace(FPSTR(T_v), _wm->configPortalActive ? _wm->_apName : (_wm->getWiFiHostname() + " - " + WiFi.localIP().toString()));
  page += str;
  page += FPSTR(HTTP_PORTAL_OPTIONS);
  page += getMenuOut();
  reportStatus(page);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);
  if(_wm->_preloadwifiscan) _wm->WiFi_scanNetworks(_wm->_scancachetime);
}

void WiFiManagerHandlers::handleWifi(AsyncWebServerRequest *request, boolean scan) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Wifi"));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("handleWifi called, scan="), scan ? "true" : "false");
  #endif
  if (captivePortal(request)) {
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("Captive portal redirect"));
    #endif
    return;
  }
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titlewifi), FPSTR(C_wifi));
  if (scan) {
    bool forceRefresh = false;
    if (request->hasParam("refresh")) {
      forceRefresh = true;
    }
    
    page += F("<div id=\"scan-results\">");
    page += getScanItemOut();
    page += F("</div>");
    
    if(forceRefresh || !_wm->_lastscan || (millis()-_wm->_lastscan > _wm->_scancachetime)){
      if(!_wm->_scanInProgress){
        _wm->WiFi_scanNetworks(true);
      } else {
        _wm->_scanRequested = true;
      }
    }
  }
  String pitem = "";

  pitem = FPSTR(HTTP_FORM_START);
  pitem.replace(FPSTR(T_v), F("wifisave"));
  page += pitem;

  pitem = FPSTR(HTTP_FORM_WIFI);
  pitem.replace(FPSTR(T_v), _wm->WiFi_SSID());

  if(_wm->_showPassword){
    pitem.replace(FPSTR(T_p), _wm->WiFi_psk());
  }
  else if(_wm->WiFi_psk() != ""){
    pitem.replace(FPSTR(T_p), FPSTR(S_passph));    
  }
  else {
    pitem.replace(FPSTR(T_p), "");    
  }

  page += pitem;

  page += getStaticOut();
  page += FPSTR(HTTP_FORM_WIFI_END);
  if(_wm->_paramsInWifi && _wm->_paramsCount > 0){
    page += FPSTR(HTTP_FORM_PARAM_HEAD);
    page += getParamOut();
  }
  page += FPSTR(HTTP_FORM_END);
  page += F("<br/><div class=\"c\"><button id=\"refresh-btn\" onclick=\"refreshScan()\">Refresh</button></div>");
  if(_wm->_showBack) page += FPSTR(HTTP_BACKBTN);
  reportStatus(page);
  
  page += getHTTPEnd();

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Page length: "), String(page.length()));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_numNetworks: "), String(_wm->_numNetworks));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_scanInProgress: "), _wm->_scanInProgress ? "true" : "false");
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("_lastscan: "), String(_wm->_lastscan));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("About to send response"));
  #endif

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Response sent"));
  #endif
}

void WiFiManagerHandlers::handleParam(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Param"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titleparam), FPSTR(C_param));

  String pitem = "";

  pitem = FPSTR(HTTP_FORM_START);
  pitem.replace(FPSTR(T_v), F("paramsave"));
  page += pitem;

  page += getParamOut();
  page += FPSTR(HTTP_FORM_END);
  if(_wm->_showBack) page += FPSTR(HTTP_BACKBTN);
  reportStatus(page);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent param page"));
  #endif
}

void WiFiManagerHandlers::handleWifiSave(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi save "));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Method:"), request->method() == HTTP_GET ? FPSTR(S_GET) : FPSTR(S_POST));
  #endif
  handleRequest(request);

  WiFiManager::WiFiManagerRequestArgs requestArgs(request);

  if (request->hasParam("s", true)) {
    _wm->_ssid = request->getParam("s", true)->value().c_str();
  }
  if (request->hasParam("p", true)) {
    _wm->_pass = request->getParam("p", true)->value().c_str();
  }

  if(_wm->_ssid == "" && _wm->_pass != ""){
    _wm->_ssid = _wm->WiFi_SSID(true);
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("Detected WiFi password change"));
    #endif    
  }

  #ifdef WM_DEBUG_LEVEL
  String requestinfo = "SERVER_REQUEST\n----------------\n";
  requestinfo += "URI: ";
  requestinfo += request->url();
  requestinfo += "\nMethod: ";
  requestinfo += (request->method() == HTTP_GET) ? "GET" : "POST";
  requestinfo += "\nArguments: ";
  requestinfo += request->params();
  requestinfo += "\n";
    for (size_t i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      requestinfo += " " + p->name() + ": " + p->value() + "\n";
    }

  _wm->DEBUG_WM(WM_DEBUG_MAX, requestinfo);
  #endif

  if (request->hasParam(FPSTR(S_ip), true)) {
    String ip = request->getParam(FPSTR(S_ip), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_ip, ip.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static ip:"), ip);
    #endif
  }
  if (request->hasParam(FPSTR(S_gw), true)) {
    String gw = request->getParam(FPSTR(S_gw), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_gw, gw.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static gateway:"), gw);
    #endif
  }
  if (request->hasParam(FPSTR(S_sn), true)) {
    String sn = request->getParam(FPSTR(S_sn), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_sn, sn.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static netmask:"), sn);
    #endif
  }
  if (request->hasParam(FPSTR(S_dns), true)) {
    String dns = request->getParam(FPSTR(S_dns), true)->value();
    _wm->optionalIPFromString(&_wm->_sta_static_dns, dns.c_str());
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_DEV, F("static DNS:"), dns);
    #endif
  }

  if (_wm->_presavewificallback != NULL) {
    _wm->_presavewificallback();
  }

  if(_wm->_paramsInWifi) doParamSave(requestArgs);

  String page;

  if(_wm->_ssid == ""){
    page = getHTTPHead(FPSTR(S_titlewifisettings), FPSTR(C_wifi));
    page += FPSTR(HTTP_PARAMSAVED);
  }
  else {
    page = getHTTPHead(FPSTR(S_titlewifisaved), FPSTR(C_wifi));
    page += FPSTR(HTTP_SAVED);
  }

  if(_wm->_showBack) page += FPSTR(HTTP_BACKBTN);
  page += getHTTPEnd();

  AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(HTTP_HEAD_CT), page);
  response->addHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL));
  request->send(response);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent wifi save page"));
  #endif

  _wm->connect = true;
}

void WiFiManagerHandlers::handleParamSave(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Param save "));
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Method:"), request->method() == HTTP_GET ? FPSTR(S_GET) : FPSTR(S_POST));
  #endif
  handleRequest(request);

  WiFiManager::WiFiManagerRequestArgs requestArgs(request);

  doParamSave(requestArgs);

  String page = getHTTPHead(FPSTR(S_titleparamsaved), FPSTR(C_param));
  page += FPSTR(HTTP_PARAMSAVED);
  if(_wm->_showBack) page += FPSTR(HTTP_BACKBTN); 
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent param save page"));
  #endif
}

void WiFiManagerHandlers::doParamSave(WiFiManager::WiFiManagerRequestArgs requestArgs){
  if ( _wm->_presaveparamscallback != NULL) {
    _wm->_presaveparamscallback();
  }

  if(_wm->_paramsCount > 0){
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,F("Parameters"));
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,FPSTR(D_HR));
    #endif

    for (int i = 0; i < _wm->_paramsCount; i++) {
      if (_wm->_params[i] == NULL || _wm->_params[i]->getValueLength() > 99999) {
        #ifdef WM_DEBUG_LEVEL
        _wm->DEBUG_WM(WM_DEBUG_ERROR,F("[ERROR] WiFiManagerParameter is out of scope"));
        #endif
        break;
      }
      
      String value;
      
      if (_wm->_params[i]->getID() == nullptr) {
        String name = (String)FPSTR(S_parampre)+(String)i;
        
        if (requestArgs.hasArg(name)) {
          value = requestArgs.getArg(name);
        } else {
          continue;
        }
      } else {
        String name = (String)FPSTR(S_parampre)+(String)i;
        if(requestArgs.hasArg(name)) {
          value = requestArgs.getArg(name);
        } else {
          value = requestArgs.getArg(_wm->_params[i]->getID());
        }
      }

      _wm->_params[i]->setValue(value.c_str(), value.length());
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_VERBOSE,(String)_wm->_params[i]->getID() + ":",value);
      #endif
    }
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE,FPSTR(D_HR));
    #endif
  }

   if ( _wm->_saveparamscallback != NULL) {
    _wm->_saveparamscallback(requestArgs);
  }
   
}

void WiFiManagerHandlers::handleInfo(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Info"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titleinfo), FPSTR(C_info));
  reportStatus(page);

  uint16_t infos = 0;

  #ifdef ESP8266
    infos = 28;
    String infoids[] = {
      F("esphead"),
      F("uptime"),
      F("chipid"),
      F("fchipid"),
      F("idesize"),
      F("flashsize"),
      F("corever"),
      F("bootver"),
      F("cpufreq"),
      F("freeheap"),
      F("memsketch"),
      F("memsmeter"),
      F("lastreset"),
      F("wifihead"),
      F("conx"),
      F("stassid"),
      F("staip"),
      F("stagw"),
      F("stasub"),
      F("dnss"),
      F("host"),
      F("stamac"),
      F("autoconx"),
      F("wifiaphead"),
      F("apssid"),
      F("apip"),
      F("apbssid"),
      F("apmac")
    };

  #elif defined(ESP32)
    infos = 27;
    String infoids[] = {
      F("esphead"),
      F("uptime"),
      F("chipid"),
      F("chiprev"),
      F("idesize"),
      F("flashsize"),      
      F("cpufreq"),
      F("freeheap"),
      F("memsketch"),
      F("memsmeter"),      
      F("lastreset"),
      F("temp"),
      F("wifihead"),
      F("conx"),
      F("stassid"),
      F("staip"),
      F("stagw"),
      F("stasub"),
      F("dnss"),
      F("host"),
      F("stamac"),
      F("apssid"),
      F("wifiaphead"),
      F("apip"),
      F("apmac"),
      F("aphost"),
      F("apbssid")
    };
  #endif

  for(size_t i=0; i<infos;i++){
    if(infoids[i] != NULL) page += getInfoData(infoids[i]);
  }
  page += F("</dl>");

  page += F("<h3>About</h3><hr><dl>");
  page += getInfoData("aboutver");
  page += getInfoData("aboutarduinover");
  page += getInfoData("aboutidfver");
  page += getInfoData("aboutdate");
  page += F("</dl>");

  if(_wm->_showInfoUpdate){
    page += HTTP_PORTAL_MENU[8];
    page += HTTP_PORTAL_MENU[9];
  }
  if(_wm->_showInfoErase) page += FPSTR(HTTP_ERASEBTN);
  if(_wm->_showBack) page += FPSTR(HTTP_BACKBTN);
  page += FPSTR(HTTP_HELP);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_DEV, F("Sent info page"));
  #endif
}

void WiFiManagerHandlers::handleExit(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Exit"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titleexit), FPSTR(C_exit));
  page += FPSTR(S_exiting);
  page += getHTTPEnd();
  AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(HTTP_HEAD_CT), page);
  response->addHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  request->send(response);
  
  _wm->_abortScheduled = true;
  _wm->_abortTime = millis() + _wm->EXIT_DELAY_MS;
}

void WiFiManagerHandlers::handleReset(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP Reset"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titlereset), FPSTR(C_restart));
  page += FPSTR(S_resetting);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(F("RESETTING ESP"));
  #endif
  _wm->_rebootScheduled = true;
  _wm->_rebootTime = millis() + _wm->REBOOT_DELAY_MS;
}

void WiFiManagerHandlers::handleErase(AsyncWebServerRequest *request, boolean opt) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_NOTIFY, F("<- HTTP Erase"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titleerase), FPSTR(C_erase));

  bool ret = _wm->erase(opt);

  if(ret) page += FPSTR(S_resetting);
  else {
    page += FPSTR(S_error);
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] WiFi EraseConfig failed"));
    #endif
  }

  page += getHTTPEnd();
  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  if(ret){
    _wm->_rebootScheduled = true;
    _wm->_rebootTime = millis() + _wm->ERASE_REBOOT_DELAY_MS;
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(F("RESETTING ESP"));
    #endif
  }	
}

void WiFiManagerHandlers::handleClose(AsyncWebServerRequest *request){
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("Disabling Captive Portal"));
  stopCaptivePortal();
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP close"));
  #endif
  handleRequest(request);
  String page = getHTTPHead(FPSTR(S_titleclose), FPSTR(C_close));
  page += FPSTR(S_closing);
  page += getHTTPEnd();
  request->send(200, FPSTR(HTTP_HEAD_CT), page);
}

void WiFiManagerHandlers::handleNotFound(AsyncWebServerRequest *request) {
  if (captivePortal(request)) return;
  handleRequest(request);
  String message = FPSTR(S_notfound);

  bool verbose404 = false;
  if(verbose404){
    message += FPSTR(S_uri);
    message += request->url();
    message += FPSTR(S_method);
    message += (request->method() == HTTP_GET) ? FPSTR(S_GET) : FPSTR(S_POST);
    message += FPSTR(S_args);
    message += request->params();
    message += F("\n");

    for (size_t i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      message += " " + p->name() + ": " + p->value() + "\n";
    }
  }
  AsyncWebServerResponse *response = request->beginResponse(404, FPSTR(HTTP_HEAD_CT2), message);
  response->addHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  response->addHeader(F("Pragma"), F("no-cache"));
  response->addHeader(F("Expires"), F("-1"));
  request->send(response);
}

void WiFiManagerHandlers::handleWiFiStatus(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi status "));
  #endif
  handleRequest(request);
  String page;
  #ifdef WM_JSTEST
    page = FPSTR(HTTP_JS);
  #endif
  request->send(200, FPSTR(HTTP_HEAD_CT), page);
}

void WiFiManagerHandlers::handleWiFiScanStatus(AsyncWebServerRequest *request){
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- HTTP WiFi scan status"));
  #endif
  handleRequest(request);
  
  String json = "{";
  json += "\"scanning\":";
  json += _wm->_scanInProgress ? "true" : "false";
  json += ",\"count\":";
  json += String(_wm->_numNetworks);
  json += ",\"lastscan\":";
  json += String(_wm->_lastscan);
  
  if(!_wm->_scanInProgress && _wm->_numNetworks > 0){
    json += ",\"networks\":[";
    
    int n = _wm->_numNetworks;
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    
    if (_wm->_removeDuplicateAPs) {
      String cssid;
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue;
        cssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++) {
          if (cssid == WiFi.SSID(indices[j])) {
            indices[j] = -1;
          }
        }
      }
    }
    
    bool first = true;
    for (int i = 0; i < n; i++) {
      if (indices[i] == -1) continue;
      
      if (!first) json += ",";
      first = false;
      
      json += "{";
      json += "\"ssid\":\"" + _wm->htmlEntities(WiFi.SSID(indices[i]), true) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(indices[i])) + ",";
      json += "\"encryption\":" + String(WiFi.encryptionType(indices[i]));
      json += "}";
    }
    json += "]";
  }
  
  json += "}";
  
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
  response->addHeader(F("Cache-Control"), F("no-cache"));
  request->send(response);
}

void WiFiManagerHandlers::handleUpdate(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Handle update"));
  #endif
  if (captivePortal(request)) return;
  String page = getHTTPHead(_wm->_title, FPSTR(C_update));
  String str = FPSTR(HTTP_ROOT_MAIN);
  str.replace(FPSTR(T_t), _wm->_title);
  str.replace(FPSTR(T_v), _wm->configPortalActive ? _wm->_apName : (_wm->getWiFiHostname() + " - " + WiFi.localIP().toString()));
  page += str;

  page += FPSTR(HTTP_UPDATE);
  page += getHTTPEnd();

  request->send(200, FPSTR(HTTP_HEAD_CT), page);
}

void WiFiManagerHandlers::handleUpdating(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static unsigned long _configPortalTimeoutSAV = 0;
  static bool timeoutSaved = false;
  
  if (!index) {
    if (!timeoutSaved) {
      _configPortalTimeoutSAV = _wm->_configPortalTimeout;
      timeoutSaved = true;
    }
    _wm->_configPortalTimeout = 0;
    
    #ifdef WM_DEBUG_LEVEL
    _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("[OTA] Update file: "), filename.c_str());
    #endif
    
    if (_wm->_preotaupdatecallback != NULL) {
      _wm->_preotaupdatecallback();
    }
    
    #ifdef ESP8266
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    #elif defined(ESP32)
      uint32_t maxSketchSpace = UPDATE_SIZE_UNKNOWN;
    #endif
    
    if (!Update.begin(maxSketchSpace)) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] OTA Update ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.begin failed");
      _wm->_configPortalTimeout = _configPortalTimeoutSAV;
      timeoutSaved = false;
      return;
    }
  }
  
  if (len) {
    if (Update.write(data, len) != len) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] OTA Update WRITE ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.write failed");
      _wm->_configPortalTimeout = _configPortalTimeoutSAV;
      timeoutSaved = false;
      return;
    }
  }
  
  if (final) {
    if (Update.end(true)) {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("\n\n[OTA] OTA FILE END bytes: "), (String)index);
      #endif
    } else {
      #ifdef WM_DEBUG_LEVEL
      _wm->DEBUG_WM(WM_DEBUG_ERROR, F("[ERROR] OTA Update END ERROR"), Update.getError());
      #endif
      request->send(500, "text/plain", "Update.end failed");
    }
    
    _wm->_configPortalTimeout = _configPortalTimeoutSAV;
    timeoutSaved = false;
  }
}

void WiFiManagerHandlers::handleUpdateDone(AsyncWebServerRequest *request) {
  #ifdef WM_DEBUG_LEVEL
  _wm->DEBUG_WM(WM_DEBUG_VERBOSE, F("<- Handle update done"));
  #endif

  String page = getHTTPHead(FPSTR(S_options), FPSTR(C_update));
  String str  = FPSTR(HTTP_ROOT_MAIN);
  str.replace(FPSTR(T_t), _wm->_title);
  str.replace(FPSTR(T_v), _wm->configPortalActive ? _wm->_apName : WiFi.localIP().toString());
  page += str;

  if (Update.hasError()) {
    page += FPSTR(HTTP_UPDATE_FAIL);
    #ifdef ESP32
    page += "OTA Error: " + (String)Update.errorString();
    #else
    page += "OTA Error: " + (String)Update.getError();
    #endif
  } else {
    page += FPSTR(HTTP_UPDATE_SUCCESS);
  }

  page += getHTTPEnd();
  request->send(200, FPSTR(HTTP_HEAD_CT), page);

  if (!Update.hasError()) {
    delay(1000);
    ESP.restart();
  }
}

#endif // defined(ESP8266) || defined(ESP32)

