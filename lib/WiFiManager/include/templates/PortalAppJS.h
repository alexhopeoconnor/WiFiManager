/**
 * PortalAppJS.h
 * Embedded SPA for WiFiManager config portal (hash routing + JSON APIs).
 */

#ifndef _WM_PORTAL_APP_JS_H_
#define _WM_PORTAL_APP_JS_H_

#include <Arduino.h>

const char PORTAL_APP_JS[] PROGMEM = R"rawliteral(
(function(){
'use strict';
var boot={};
try{var el=document.getElementById('wm-bootstrap');if(el&&el.textContent)boot=JSON.parse(el.textContent);}catch(e){boot={};}

function $(id){return document.getElementById(id);}
function esc(t){if(!t)return'';return String(t).replace(/[&<>"']/g,function(c){return({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]);});}

function setView(html){
  var a=$('app');
  if(!a)return;
  a.innerHTML="<div class='wrap'>"+html+"</div>";
}

function navBar(){
  var f=boot.features||{};
  var h="<p style='text-align:center'>";
  h+="<a href='#/'>Home</a> &middot; ";
  h+="<a href='#/wifi'>WiFi</a>";
  if(!f.paramsInWifi){h+=" &middot; <a href='#/setup'>Setup</a>";}
  h+=" &middot; <a href='#/info'>Info</a>";
  h+=" &middot; <a href='#/update'>Update</a>";
  h+="</p><hr/>";
  return h;
}

function viewHome(){
  var sub=esc(boot.subtitle||'');
  var t=esc(boot.title||'WiFiManager');
  var html=navBar();
  html+="<h1>"+t+"</h1><h3>"+sub+"</h3>";
  html+="<p>Use the links above to configure WiFi, view device info, or update firmware.</p>";
  setView(html);
}

function api(path, opt){
  opt=opt||{};
  return fetch(path,{method:opt.method||'GET',headers:opt.headers,body:opt.body,credentials:'same-origin'})
    .then(function(r){return r.text().then(function(t){return {ok:r.ok,status:r.status,body:t};});});
}

function scanPollTimer(){}

function renderScanList(data){
  if(!data||data.scanning){return '<p>Scanning for networks...</p>';}
  if(data.error==='timeout')return '<p>Scan timed out. Try Refresh.</p>';
  if(data.error==='failed')return '<p>Scan failed.</p>';
  if(!data.networks||data.count===0)return '<p>No networks found.</p>';
  var html='',i,n,q,pct,qi,enc;
  for(i=0;i<data.networks.length;i++){
    n=data.networks[i];
    pct=(n.quality||0)+'%';
    qi=Math.round((n.quality/100)*3)+1;if(qi<1)qi=1;if(qi>4)qi=4;
    enc=n.encrypted?'l':'';
    html+="<div><a href='#p' onclick='return portalPickSsid(this)' data-ssid='"+esc(n.ssid)+"'>"+esc(n.ssid)+"</a>";
    html+="<div role='img' aria-label='"+pct+"' title='"+pct+"' class='q q-"+qi+" "+enc+"'></div>";
    html+="<div class='q'>"+pct+"</div></div>";
  }
  return html;
}

window.portalPickSsid=function(el){
  var s=el.getAttribute('data-ssid');
  var inp=document.getElementById('wm-s');
  if(inp&&s){inp.value=s;}
  return false;
};

function wifiRefresh(){
  var box=document.getElementById('wm-scan-results');
  if(!box)return;
  box.innerHTML='<p>Starting scan...</p>';
  api('/api/wifi/scan',{method:'POST'}).then(function(){
    var iv=setInterval(function(){
      api('/api/wifi/scan-status').then(function(res){
        try{var d=JSON.parse(res.body);}catch(e){return;}
        if(box)box.innerHTML=renderScanList(d);
        if(!d.scanning)clearInterval(iv);
      });
    },800);
  });
}

function viewWifi(){
  setView(navBar()+"<h2>WiFi</h2><p>Loading...</p>");
  api('/api/wifi/meta').then(function(res){
    try{var m=JSON.parse(res.body);}catch(e){m={};}
    var html=navBar()+"<h2>WiFi</h2>";
    html+="<div id='wm-scan-results'></div>";
    html+="<form id='wm-wifi-form' onsubmit='return portalWifiSave(event)'>";
    html+="<label>SSID</label><input id='wm-s' name='s' maxlength='32' value='"+esc(m.ssidPlaceholder||'')+"'/>";
    html+="<label>Password</label><input id='wm-p' name='p' type='password' maxlength='64' placeholder='"+esc(m.passwordPlaceholder||'')+"'/>";
    html+="<input type='checkbox' id='wm-showpass' onclick='var p=document.getElementById(\"wm-p\");p.type=this.checked?\"text\":\"password\"'/> <label for='wm-showpass'>Show password</label>";
    if(m.staticFieldsHtml){html+=m.staticFieldsHtml;}
    if(m.paramsHtml){html+=m.paramsHtml;}
    html+="<button type='submit'>Save</button></form>";
    html+="<br/><button type='button' id='wm-refresh-scan' onclick='wifiRefresh()'>Refresh scan</button>";
    html+="<div id='wm-wifi-msg'></div>";
    setView(html);
    api('/api/wifi/scan-status').then(function(r2){
      try{var d=JSON.parse(r2.body);}catch(e){d={};}
      var box=document.getElementById('wm-scan-results');
      if(box)box.innerHTML=renderScanList(d);
      if(d&&d.scanning){wifiRefresh();}
    });
  });
}

window.portalWifiSave=function(ev){
  ev.preventDefault();
  var fd=new FormData(document.getElementById('wm-wifi-form'));
  var msg=$('wm-wifi-msg');
  if(msg)msg.innerHTML='Saving...';
  api('/api/wifi/save',{method:'POST',body:fd}).then(function(res){
    try{var j=JSON.parse(res.body);}catch(e){j={ok:false};}
    if(msg)msg.innerHTML=(j&&j.message)?esc(j.message):(res.ok?'Saved.':'Error');
  });
  return false;
};

function viewInfo(){
  setView(navBar()+"<p>Loading info...</p>");
  api('/api/info').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar()+"<h2>Info</h2>";
    function section(title,rows){
      if(!rows||!rows.length)return'';
      var h="<h3>"+esc(title)+"</h3><hr/><dl>";
      for(var i=0;i<rows.length;i++){
        h+="<dt>"+esc(rows[i].label)+"</dt><dd>"+rows[i].value+"</dd>";
      }
      h+="</dl>";
      return h;
    }
    html+=section('Status',d.status||[]);
    html+=section('Device',d.device||[]);
    html+=section('WiFi',d.wifi||[]);
    html+=section('About',d.about||[]);
    var act=d.actions||{};
    if(act.showUpdate)html+="<p><a href='#/update'>Firmware update</a></p>";
    if(act.showErase)html+="<p><button type='button' class='D' onclick='portalErase()'>Erase WiFi config</button></p>";
    setView(html);
  });
}

window.portalErase=function(){
  if(!confirm('Erase WiFi configuration?'))return;
  api('/api/device/erase',{method:'POST'}).then(function(res){
    try{var j=JSON.parse(res.body);}catch(e){j={};}
    alert((j&&j.message)?j.message:'Done');
  });
};

function viewSetup(){
  setView(navBar()+"<p>Loading...</p>");
  api('/api/params').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar()+"<h2>Parameters</h2>";
    html+="<form id='wm-param-form' onsubmit='return portalParamSave(event)'>"+(d.paramsHtml||'')+"<button type='submit'>Save</button></form>";
    html+="<div id='wm-param-msg'></div>";
    setView(html);
  });
}

window.portalParamSave=function(ev){
  ev.preventDefault();
  var fd=new FormData(document.getElementById('wm-param-form'));
  var msg=$('wm-param-msg');
  if(msg)msg.innerHTML='Saving...';
  api('/api/params/save',{method:'POST',body:fd}).then(function(res){
    try{var j=JSON.parse(res.body);}catch(e){j={};}
    if(msg)msg.innerHTML=(j&&j.message)?esc(j.message):'Saved.';
  });
  return false;
};

function viewUpdate(){
  var html=navBar()+"<h2>Firmware update</h2>";
  html+="<form method='POST' action='/u' enctype='multipart/form-data'>";
  html+="<input type='file' name='update'/>";
  html+="<button type='submit'>Update</button></form>";
  html+="<p><small>Upload a .bin firmware. Device may restart after update.</small></p>";
  setView(html);
}

function route(){
  var h=location.hash||'#/';
  if(h.indexOf('#/')!==0)h='#/';
  if(h==='#/'||h==='#'){viewHome();return;}
  if(h==='#/wifi'){viewWifi();return;}
  if(h==='#/info'){viewInfo();return;}
  if(h==='#/setup'){viewSetup();return;}
  if(h==='#/update'){viewUpdate();return;}
  viewHome();
}

window.addEventListener('hashchange',route);
window.addEventListener('load',route);
})();

)rawliteral";

#endif  // _WM_PORTAL_APP_JS_H_
