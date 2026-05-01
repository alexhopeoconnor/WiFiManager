/**
 * PortalAppJS.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
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

var _wmToastTimer=null;
function showToast(msg,isErr){
  var el=$('wm-toast');
  if(!el)return;
  el.textContent=msg||'';
  el.className='wm-toast '+(isErr?'err':'ok');
  el.style.display='block';
  if(_wmToastTimer)clearTimeout(_wmToastTimer);
  _wmToastTimer=setTimeout(function(){el.style.display='none';el.textContent='';},4500);
}
function showConfirm(msg,onOk){
  var dlg=$('wm-dialog'),dmsg=$('wm-dialog-msg'),bk=$('wm-dialog-backdrop'),ok=$('wm-dialog-ok'),cancel=$('wm-dialog-cancel');
  if(!dlg||!dmsg||!ok||!cancel){if(onOk)onOk();return;}
  dmsg.textContent=msg||'';
  dlg.style.display='flex';
  dlg.setAttribute('aria-hidden','false');
  function hide(){
    dlg.style.display='none';
    dlg.setAttribute('aria-hidden','true');
    ok.onclick=null;
    cancel.onclick=null;
    if(bk)bk.onclick=null;
  }
  function onCancel(){hide();}
  cancel.onclick=onCancel;
  if(bk)bk.onclick=onCancel;
  ok.onclick=function(){hide();if(onOk)onOk();};
}

function setView(html){
  var a=$('app');
  if(!a)return;
  stopWifiScanPolling();
  a.innerHTML="<div class='wrap'>"+html+"</div>";
}

function renderMetaField(f){
  if(!f)return'';
  if(f.html)return f.html;
  var id=f.id||f.name||'';
  var nm=f.name||f.id||'';
  var domId=(id==='s')?'wm-s':(id==='p')?'wm-p':('wm-f-'+id);
  var t=f.type||'text';
  var h='<label for="'+domId+'">'+esc(f.label||'')+'</label>';
  h+='<input id="'+domId+'" name="'+esc(nm)+'" type="'+esc(t)+'"';
  if(f.maxlength)h+=' maxlength="'+f.maxlength+'"';
  if(f.value!==undefined&&f.value!==null)h+=' value="'+esc(f.value)+'"';
  if(f.placeholder)h+=' placeholder="'+esc(f.placeholder)+'"';
  if(f.customAttrs)h+=' '+f.customAttrs;
  h+='/>';
  return h;
}

function renderFieldList(arr){
  if(!arr||!arr.length)return'';
  var h='',i;
  for(i=0;i<arr.length;i++)h+=renderMetaField(arr[i]);
  return h;
}

function navBar(){
  var f=boot.features||{};
  var h="<p style='text-align:center'>";
  h+="<a href='#/'>Home</a> &middot; ";
  h+="<a href='#/wifi'>WiFi</a>";
  if(!f.paramsInWifi){h+=" &middot; <a href='#/setup'>Setup</a>";}
  if(f.showInfo!==false){h+=" &middot; <a href='#/info'>Info</a>";}
  if(f.showUpdate){h+=" &middot; <a href='#/update'>Update</a>";}
  if(boot.showBack){h+=" &middot; <a href='#' onclick='history.back();return false;'>Back</a>";}
  h+="</p><hr/>";
  return h;
}

function deviceActionsHtml(){
  var f=boot.features||{};
  var h='', ok=false;
  if(f.showRestart){h+="<p><button type='button' onclick='portalRestart()'>Restart</button></p>";ok=true;}
  if(f.showExitPortal){h+="<p><button type='button' onclick='portalExit()'>Exit portal</button></p>";ok=true;}
  if(f.showCloseCaptive){h+="<p><button type='button' onclick='portalCloseCaptive()'>Stop captive portal detection</button></p>";ok=true;}
  if(!ok)return'';
  return "<div class='device-actions'><h3>Device actions</h3>"+h+"</div>";
}

function viewHome(){
  var sub=esc(boot.subtitle||'');
  var t=esc(boot.title||'WiFiManager');
  var html=navBar();
  html+="<h1>"+t+"</h1><h3>"+sub+"</h3>";
  if(boot.initialStatus)html+="<p class='status'>"+esc(boot.initialStatus)+"</p>";
  html+="<p>Use the links above to configure WiFi, view device info, or update firmware.</p>";
  html+=deviceActionsHtml();
  setView(html);
}

function api(path, opt){
  opt=opt||{};
  return fetch(path,{method:opt.method||'GET',headers:opt.headers,body:opt.body,credentials:'same-origin'})
    .then(function(r){return r.text().then(function(t){return {ok:r.ok,status:r.status,body:t};});});
}

var _wmWifiScanPollTimer=null;

function stopWifiScanPolling(){
  if(_wmWifiScanPollTimer){
    clearInterval(_wmWifiScanPollTimer);
    _wmWifiScanPollTimer=null;
  }
}

function startWifiScanPolling(box){
  stopWifiScanPolling();
  _wmWifiScanPollTimer=setInterval(function(){
    api('/api/wifi/scan-status').then(function(res){
      var d={};
      try{d=JSON.parse(res.body);}catch(e){return;}
      if(box)box.innerHTML=renderScanList(d);
      if(!d.scanning)stopWifiScanPolling();
    });
  },800);
}

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
    html+="<div><a href='#p' data-ssid='"+esc(n.ssid)+"'>"+esc(n.ssid)+"</a>";
    html+="<div role='img' aria-label='"+pct+"' title='"+pct+"' class='q q-"+qi+" "+enc+"'></div>";
    html+="<div class='q'>"+pct+"</div></div>";
  }
  return html;
}

function wifiRefresh(){
  var box=document.getElementById('wm-scan-results');
  if(!box)return;
  box.innerHTML='<p>Starting scan...</p>';
  api('/api/wifi/scan',{method:'POST'}).then(function(){
    startWifiScanPolling(box);
  });
}

function bindWifiViewEvents(){
  var refreshBtn=$('wm-refresh-scan');
  if(refreshBtn){
    refreshBtn.addEventListener('click', wifiRefresh);
  }

  var showPass=$('wm-showpass');
  if(showPass){
    showPass.addEventListener('change', function(){
      var p=$('wm-p');
      if(p)p.type=this.checked?'text':'password';
    });
  }

  var scanResults=$('wm-scan-results');
  if(scanResults){
    scanResults.addEventListener('click', function(ev){
      var el=ev.target;
      while(el && el !== scanResults){
        if(el.tagName==='A' && el.getAttribute('data-ssid')){
          ev.preventDefault();
          var inp=$('wm-s');
          if(inp)inp.value=el.getAttribute('data-ssid')||'';
          return;
        }
        el=el.parentElement;
      }
    });
  }
}

function viewWifi(){
  setView(navBar()+"<h2>WiFi</h2><p>Loading...</p>");
  api('/api/wifi/meta').then(function(res){
    try{var m=JSON.parse(res.body);}catch(e){m={};}
    var html=navBar()+"<h2>WiFi</h2>";
    html+="<div id='wm-scan-results'></div>";
    html+="<form id='wm-wifi-form' onsubmit='return portalWifiSave(event)'>";
    html+=renderFieldList(m.wifiFields||[]);
    var wf=m.wifiFields||[];
    var hasPass=false;
    var i;
    for(i=0;i<wf.length;i++){if(wf[i]&&wf[i].id==='p'){hasPass=true;break;}}
    if(hasPass){
      html+="<input type='checkbox' id='wm-showpass'/> <label for='wm-showpass'>Show password</label>";
    }
    html+=renderFieldList(m.staticFields||[]);
    html+=renderFieldList(m.params||[]);
    html+="<button type='submit'>Save</button></form>";
    html+="<br/><button type='button' id='wm-refresh-scan'>Refresh scan</button>";
    html+="<div id='wm-wifi-msg'></div>";
    setView(html);
    bindWifiViewEvents();
    api('/api/wifi/scan-status').then(function(r2){
      try{var d=JSON.parse(r2.body);}catch(e){d={};}
      var box=document.getElementById('wm-scan-results');
      if(box)box.innerHTML=renderScanList(d);
      if(d&&d.scanning){startWifiScanPolling(box);}
    });
  });
}

function pollWifiConnectStatus(){
  var msg=$('wm-wifi-msg');
  var iv=setInterval(function(){
    api('/api/wifi/connect-status').then(function(res){
      var j={};
      try{j=JSON.parse(res.body);}catch(e){}
      if(msg&&j.message){
        msg.innerHTML=esc(j.message);
      }
      if(j.state==='success'){
        clearInterval(iv);
        showToast(j.message||'WiFi connected',false);
        location.hash='#/';
      }else if(j.state==='failed'){
        clearInterval(iv);
        showToast(j.message||'WiFi connect failed',true);
      }
    });
  },700);
}
window.portalWifiSave=function(ev){
  ev.preventDefault();
  var fd=new FormData(document.getElementById('wm-wifi-form'));
  var msg=$('wm-wifi-msg');
  if(msg)msg.innerHTML='Submitting...';
  api('/api/wifi/save',{method:'POST',body:fd}).then(function(res){
    var j={};
    try{j=JSON.parse(res.body);}catch(e){}
    if(msg){
      msg.innerHTML=(j&&j.message)?esc(j.message):(res.ok?'Queued.':'Error');
    }
    if(res.status===202){
      pollWifiConnectStatus();
    }
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
    var st=d.status;
    if(st && typeof st==='object' && !Array.isArray(st)){
      html+="<h3>Status</h3><hr/><dl>";
      html+="<dt>Connected</dt><dd>"+esc(String(st.connected))+"</dd>";
      if(st.ssid!==undefined)html+="<dt>SSID</dt><dd>"+esc(st.ssid)+"</dd>";
      if(st.stationIp)html+="<dt>Station IP</dt><dd>"+esc(st.stationIp)+"</dd>";
      if(st.apIp)html+="<dt>AP IP</dt><dd>"+esc(st.apIp)+"</dd>";
      if(st.summary)html+="<dt>Summary</dt><dd>"+esc(st.summary)+"</dd>";
      html+="</dl>";
    } else {
      html+=section('Status',d.status||[]);
    }
    html+=section('Device',d.device||[]);
    html+=section('WiFi',d.wifi||[]);
    html+=section('About',d.about||[]);
    var act=d.actions||{};
    html+="<h3>Actions</h3><hr/>";
    if(act.showRestart)html+="<p><button type='button' onclick='portalRestart()'>Restart</button></p>";
    if(act.showExitPortal)html+="<p><button type='button' onclick='portalExit()'>Exit portal</button></p>";
    if(act.showCloseCaptive)html+="<p><button type='button' onclick='portalCloseCaptive()'>Stop captive portal detection</button></p>";
    if(act.showUpdate)html+="<p><a href='#/update'>Firmware update</a></p>";
    if(act.showErase)html+="<p><button type='button' class='D' onclick='portalErase()'>Erase WiFi config</button></p>";
    setView(html);
  });
}

function portalConfirmPost(url, promptMsg){
  showConfirm(promptMsg||'Continue?',function(){
    api(url,{method:'POST'}).then(function(res){
      try{var j=JSON.parse(res.body);}catch(e){j={};}
      var m=(j&&j.message)?j.message:(res.ok?'OK':'Error');
      showToast(m,!res.ok);
    });
  });
}

window.portalRestart=function(){portalConfirmPost('/api/device/restart','Restart the device?');};
window.portalExit=function(){portalConfirmPost('/api/portal/exit','Exit the configuration portal?');};
window.portalCloseCaptive=function(){portalConfirmPost('/api/portal/close','Stop redirecting clients to this portal (captive detection off)?');};

window.portalErase=function(){
  showConfirm('Erase WiFi configuration?',function(){
    api('/api/device/erase',{method:'POST'}).then(function(res){
      try{var j=JSON.parse(res.body);}catch(e){j={};}
      showToast((j&&j.message)?j.message:(res.ok?'Done':'Error'),!res.ok);
    });
  });
};

function viewSetup(){
  setView(navBar()+"<p>Loading...</p>");
  api('/api/params').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar()+"<h2>Parameters</h2>";
    html+="<form id='wm-param-form' onsubmit='return portalParamSave(event)'>";
    html+=renderFieldList(d.params||[]);
    html+="<button type='submit'>Save</button></form>";
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
  var f=boot.features||{};
  var html=navBar()+"<h2>Firmware update</h2>";
  if(!f.showUpdate){html+="<p>Firmware update is disabled.</p>";setView(html);return;}
  html+="<form id='wm-ota-form' onsubmit='return portalOtaSubmit(event)'>";
  html+="<input type='file' id='wm-ota-file' name='update'/>";
  html+="<button type='submit'>Upload</button></form>";
  html+="<p><small>Upload a .bin firmware. The device restarts after a successful update.</small></p>";
  html+="<pre id='wm-ota-log'></pre>";
  setView(html);
}

window.portalOtaSubmit=function(ev){
  ev.preventDefault();
  var fi=$('wm-ota-file');
  var logEl=$('wm-ota-log');
  if(!fi||!fi.files||!fi.files[0]){
    if(logEl)logEl.textContent='Choose a firmware file first.';
    return false;
  }
  if(logEl)logEl.textContent='Uploading…';
  var xhr=new XMLHttpRequest();
  xhr.upload.onprogress=function(e){
    if(!logEl||!e.lengthComputable)return;
    logEl.textContent='Uploading… '+Math.round(100*e.loaded/e.total)+'%';
  };
  xhr.onreadystatechange=function(){
    if(xhr.readyState!==4)return;
    var t=xhr.responseText||'';
    if(logEl)logEl.textContent=t;
    var httpOk=xhr.status>=200&&xhr.status<300;
    try{
      var j=JSON.parse(t);
      if(j&&j.message){
        if(logEl)logEl.textContent=j.message;
        showToast(j.message,j.ok===false);
        return;
      }
    }catch(e){}
    showToast(httpOk?'Upload finished.':('HTTP '+xhr.status),!httpOk);
  };
  xhr.open('POST','/u');
  var fd=new FormData();
  fd.append('update',fi.files[0]);
  xhr.send(fd);
  return false;
};

function route(){
  var h=location.hash||'#/';
  if(h.indexOf('#/')!==0)h='#/';
  var f=boot.features||{};
  if(h==='#/info'&&f.showInfo===false){viewHome();return;}
  if(h==='#/update'&&f.showUpdate===false){viewHome();return;}
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
