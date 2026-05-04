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

// --- Core / DOM ---
function $(id){return document.getElementById(id);}
function esc(t){if(!t)return'';return String(t).replace(/[&<>"']/g,function(c){return({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]);});}

// --- Toast / dialog ---
var _wmToastTimer=null;
var _wmPortalTimeoutTimer=null;
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

function bindPortalChromeEvents(){
  var back=$('wm-nav-back');
  if(back){
    back.addEventListener('click', function(ev){ ev.preventDefault(); history.back(); });
  }
  var r=$('wm-restart-btn'), e=$('wm-exit-btn'), c=$('wm-close-captive-btn'), er=$('wm-erase-btn');
  if(r && window.portalRestart) r.addEventListener('click', window.portalRestart);
  if(e && window.portalExit) e.addEventListener('click', window.portalExit);
  if(c && window.portalCloseCaptive) c.addEventListener('click', window.portalCloseCaptive);
  if(er && window.portalErase) er.addEventListener('click', window.portalErase);
}

function bindFormSubmitHandlers(){
  var wf=$('wm-wifi-form'), pf=$('wm-param-form'), of=$('wm-ota-form');
  if(wf && window.portalWifiSave) wf.addEventListener('submit', window.portalWifiSave);
  if(pf && window.portalParamSave) pf.addEventListener('submit', window.portalParamSave);
  if(of && window.portalOtaSubmit) of.addEventListener('submit', window.portalOtaSubmit);
}

// --- View shell ---
function stopPortalTimeoutCountdown(){
  if(_wmPortalTimeoutTimer){
    clearInterval(_wmPortalTimeoutTimer);
    _wmPortalTimeoutTimer=null;
  }
}

function setView(html){
  var a=$('app');
  if(!a)return;
  stopPortalTimeoutCountdown();
  stopWifiScanPolling();
  stopWifiConnectPolling();
  a.innerHTML="<div class='wm-layout'>"+html+"</div>";
  bindPortalChromeEvents();
  bindFormSubmitHandlers();
}

function formatDurationSeconds(totalSeconds){
  totalSeconds=Math.max(0,Math.floor(totalSeconds||0));
  var hours=Math.floor(totalSeconds/3600);
  var minutes=Math.floor((totalSeconds%3600)/60);
  var seconds=totalSeconds%60;
  if(hours>0){
    return hours+":"+String(minutes).padStart(2,'0')+":"+String(seconds).padStart(2,'0');
  }
  return minutes+":"+String(seconds).padStart(2,'0');
}

function startPortalTimeoutCountdown(initialSeconds){
  var el=$('wm-portal-timeout');
  var remaining=Math.max(0,Math.floor(initialSeconds||0));
  if(!el||remaining<=0)return;
  function render(){
    if(!el)return;
    el.textContent='Captive portal timeout: '+formatDurationSeconds(remaining);
  }
  render();
  _wmPortalTimeoutTimer=setInterval(function(){
    remaining=Math.max(0,remaining-1);
    render();
    if(remaining<=0)stopPortalTimeoutCountdown();
  },1000);
}

// --- Field render (WiFi / params meta) ---
function renderStandardField(f){
  var id=f.id||f.name||'';
  var nm=f.name||f.id||'';
  var domId=(id==='s')?'wm-s':(id==='p')?'wm-p':('wm-f-'+id);
  var t=f.type||'text';
  var h='<div class="wm-field">';
  h+='<label for="'+domId+'">'+esc(f.label||'')+'</label>';
  h+='<input id="'+domId+'" name="'+esc(nm)+'" type="'+esc(t)+'"';
  if(f.maxlength)h+=' maxlength="'+f.maxlength+'"';
  if(f.value!==undefined&&f.value!==null)h+=' value="'+esc(f.value)+'"';
  if(f.placeholder)h+=' placeholder="'+esc(f.placeholder)+'"';
  if(f.customAttrs)h+=' '+f.customAttrs;
  h+='/>';
  h+='</div>';
  return h;
}

function renderMetaField(f){
  if(!f)return'';
  if(f.kind==='html')return f.html||'';
  if(f.kind==='field' || !f.kind)return renderStandardField(f);
  return '';
}

function renderFieldList(arr){
  if(!arr||!arr.length)return'';
  var h='',i;
  for(i=0;i<arr.length;i++)h+=renderMetaField(arr[i]);
  return h;
}

function navBar(active){
  active=active||'home';
  var pages=boot.pages||{};
  var layout=boot.layout||{};
  var actions=boot.actions||{};
  var pinWifi=(layout.paramsLocation||'wifi')==='wifi';
  var ps=pages.setup||{};
  var pi=pages.info||{};
  var pu=pages.update||{};
  var h="<nav class='wm-nav' aria-label='Configuration'>";
  h+="<a class='wm-nav-link"+(active==='home'?' wm-nav-link--active':'')+"' href='#/'>Home</a>";
  h+="<a class='wm-nav-link"+(active==='wifi'?' wm-nav-link--active':'')+"' href='#/wifi'>WiFi</a>";
  if(!pinWifi && ps.visible!==false){h+="<a class='wm-nav-link"+(active==='setup'?' wm-nav-link--active':'')+"' href='#/setup'>Setup</a>";}
  if(pi.visible!==false){h+="<a class='wm-nav-link"+(active==='info'?' wm-nav-link--active':'')+"' href='#/info'>Info</a>";}
  if(pu.visible){h+="<a class='wm-nav-link"+(active==='update'?' wm-nav-link--active':'')+"' href='#/update'>Update</a>";}
  if(actions.back&&actions.back.visible){h+="<a href='#' class='wm-nav-link' id='wm-nav-back'>Back</a>";}
  h+="</nav>";
  return h;
}

function deviceActionsHtml(){
  var actions=boot.actions||{};
  var h='', ok=false;
  if(actions.restart&&actions.restart.visible){h+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-restart-btn'>Restart device</button>";ok=true;}
  if(actions.exitPortal&&actions.exitPortal.visible){h+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-exit-btn'>Exit portal</button>";ok=true;}
  if(actions.closeCaptive&&actions.closeCaptive.visible){h+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-close-captive-btn'>Stop captive portal</button>";ok=true;}
  if(!ok)return'';
  return "<div class='wm-card'><h2 class='wm-card-title'>Device</h2><div class='wm-btn-group'>"+h+"</div></div>";
}

function renderExtraHomeCards(cards){
  if(!cards||!cards.length)return'';
  var out='',i,c,j,k,cls;
  for(i=0;i<cards.length;i++){
    c=cards[i];
    if(!c)continue;
    out+="<div class='wm-card'>";
    if(c.title){out+="<h2 class='wm-card-title'>"+esc(c.title)+"</h2>";}
    if(c.kind==='text'||c.kind==='callout'){
      cls=c.kind==='callout'?'wm-callout wm-callout--info':'wm-lead';
      out+="<p class='"+cls+"'>"+esc(c.text||'')+"</p>";
    }else{
      out+="<dl class='wm-kv'>";
      if(c.items){for(j=0;j<c.items.length;j++){k=c.items[j];out+="<dt>"+esc(k.label)+"</dt><dd>"+esc(k.value)+"</dd>";}}
      out+="</dl>";
    }
    out+="</div>";
  }
  return out;
}

function viewHome(){
  var brand=boot.brand||{};
  var ctx=boot.context||{};
  var t=esc(brand.title||'WiFiManager');
  var ident=ctx.identityText?esc(ctx.identityText):'';
  var summary=ctx.statusSummary||'';
  var timeoutSeconds=ctx.portalTimeoutSecondsRemaining||0;
  var html=navBar('home');
  html+="<header class='wm-hero'>";
  if(brand.logoSvg){html+="<div class='wm-brand-logo'>"+brand.logoSvg+"</div>";}
  html+="<h1 class='wm-hero-title'>"+t+"</h1>";
  if(ident){html+="<p class='wm-hero-sub'>"+ident+"</p>";}
  if(brand.homeIntro){html+="<p class='wm-hero-intro'>"+esc(brand.homeIntro)+"</p>";}
  html+="</header>";
  html+="<div class='wm-card'><h2 class='wm-card-title'>Status</h2>";
  if(summary){html+="<p class='wm-home-summary'>"+esc(summary)+"</p>";}
  if(timeoutSeconds>0){html+="<p class='wm-status' id='wm-portal-timeout'></p>";}
  html+="<p class='wm-lead'>Use <strong>WiFi</strong> to scan networks and connect. Open <strong>Info</strong> for addresses and firmware actions.</p>";
  html+="</div>";
  html+=renderExtraHomeCards(boot.extraHomeCards||[]);
  html+=deviceActionsHtml();
  setView(html);
  if(timeoutSeconds>0)startPortalTimeoutCountdown(timeoutSeconds);
}

// --- HTTP ---
function api(path, opt){
  opt=opt||{};
  return fetch(path,{method:opt.method||'GET',headers:opt.headers,body:opt.body,credentials:'same-origin'})
    .then(function(r){return r.text().then(function(t){return {ok:r.ok,status:r.status,body:t};});});
}

// --- WiFi scan view ---
var _wmWifiScanPollTimer=null;
var _wmWifiScanPollToken=0;

function getScanResultsBox(){
  return $('wm-scan-results');
}

function setScanRefreshEnabled(enabled){
  var btn=$('wm-refresh-scan');
  if(btn)btn.disabled=(enabled===false);
}

function stopWifiScanPolling(){
  _wmWifiScanPollToken++;
  if(_wmWifiScanPollTimer){
    clearTimeout(_wmWifiScanPollTimer);
    _wmWifiScanPollTimer=null;
  }
  setScanRefreshEnabled(true);
}

function startWifiScanPolling(box){
  stopWifiScanPolling();
  setScanRefreshEnabled(false);
  var token=++_wmWifiScanPollToken;
  function poll(){
    api('/api/wifi/scan-status').then(function(res){
      if(token!==_wmWifiScanPollToken)return;
      var d={};
      try{d=JSON.parse(res.body);}catch(e){return;}
      box=getScanResultsBox()||box;
      if(box)box.innerHTML=renderScanList(d);
      if(!d.scanning){
        stopWifiScanPolling();
        return;
      }
      _wmWifiScanPollTimer=setTimeout(poll,800);
    }).catch(function(){
      if(token!==_wmWifiScanPollToken)return;
      stopWifiScanPolling();
      box=getScanResultsBox()||box;
      if(box)box.innerHTML=renderScanError('Portal became unreachable while scanning.');
    });
  }
  poll();
}

function renderScanList(data){
  if(!data||data.scanning){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Scanning for networks…</p></div>";
  }
  if(data.error==='timeout'){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Scan timed out. Try Scan again.</p></div>";
  }
  if(data.error==='failed'){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Scan failed.</p></div>";
  }
  if(!data.networks||data.count===0){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>No networks found.</p></div>";
  }
  var html="<div class='wm-scan-list'>",i,n,pct,qi,enc;
  for(i=0;i<data.networks.length;i++){
    n=data.networks[i];
    pct=(n.quality||0)+'%';
    qi=Math.round((n.quality/100)*3)+1;if(qi<1)qi=1;if(qi>4)qi=4;
    html+="<a class='wm-scan-row' href='#p' data-ssid='"+esc(n.ssid)+"'>";
    html+="<span class='wm-scan-ssid'>"+esc(n.ssid)+"</span>";
    html+="<span class='wm-scan-meta'>";
    if(n.encrypted){html+="<span class='wm-scan-lock q l' aria-hidden='true'></span>";}
    html+="<span role='img' aria-label='"+pct+"' title='"+pct+"' class='wm-scan-bars q q-"+qi+"'></span>";
    html+="<span class='wm-scan-signal'>"+pct+"</span>";
    html+="</span>";
    html+="</a>";
  }
  html+="</div>";
  return html;
}

function renderScanError(msg){
  return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>"+esc(msg||'Failed to reach portal while scanning.')+"</p></div>";
}

function wifiRefresh(){
  var box=getScanResultsBox();
  if(!box)return;
  setScanRefreshEnabled(false);
  box.innerHTML="<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Starting scan…</p></div>";
  api('/api/wifi/scan',{method:'POST'}).then(function(res){
    box=getScanResultsBox()||box;
    if(!res.ok){
      setScanRefreshEnabled(true);
      if(box)box.innerHTML=renderScanError('Failed to start WiFi scan.');
      showToast('Failed to start WiFi scan.',true);
      return;
    }
    startWifiScanPolling(box);
  }).catch(function(){
    stopWifiScanPolling();
    box=getScanResultsBox()||box;
    if(box)box.innerHTML=renderScanError('Failed to reach portal while starting scan.');
    showToast('Failed to reach portal while starting scan.',true);
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
  setView(navBar('wifi')+"<div class='wm-card'><p class='wm-lead'>Loading WiFi options…</p></div>");
  api('/api/wifi/meta').then(function(res){
    try{var m=JSON.parse(res.body);}catch(e){m={};}
    var html=navBar('wifi');
    html+="<div class='wm-page-head'><h1>WiFi</h1><p class='wm-page-desc'>Pick a network below, confirm SSID and password, then connect.</p></div>";
    html+="<div class='wm-card'><h2 class='wm-card-title'>Networks nearby</h2>";
    html+="<div id='wm-scan-results'></div>";
    html+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-refresh-scan'>Scan again</button></div>";
    html+="<div class='wm-card'><h2 class='wm-card-title'>Network details</h2>";
    html+="<form id='wm-wifi-form'>";
    html+=renderFieldList(m.wifiFields||[]);
    var wf=m.wifiFields||[];
    var hasPass=false;
    var i;
    for(i=0;i<wf.length;i++){if(wf[i]&&wf[i].id==='p'){hasPass=true;break;}}
    if(hasPass){
      html+="<div class='wm-checkbox-row'><input type='checkbox' id='wm-showpass'/> <label for='wm-showpass'>Show password</label></div>";
    }
    html+=renderFieldList(m.staticFields||[]);
    html+=renderFieldList(m.params||[]);
    html+="<div class='wm-form-actions'><button type='submit' class='wm-btn wm-btn--primary wm-btn--block'>Save and connect</button></div></form>";
    html+="<div id='wm-wifi-msg'></div></div>";
    setView(html);
    bindWifiViewEvents();
    setScanRefreshEnabled(true);
    api('/api/wifi/scan-status').then(function(r2){
      try{var d=JSON.parse(r2.body);}catch(e){d={};}
      var box=getScanResultsBox();
      if(box)box.innerHTML=renderScanList(d);
      if(d&&d.scanning){startWifiScanPolling(box);}
    }).catch(function(){
      var box=getScanResultsBox();
      if(box)box.innerHTML=renderScanError('Failed to reach portal while loading scan status.');
      setScanRefreshEnabled(true);
    });
  }).catch(function(){
    setView(navBar('wifi')+"<div class='wm-card'><p class='wm-lead'>Failed to load WiFi options. Reconnect to the portal and try again.</p></div>");
  });
}

// --- WiFi connect polling ---
var _wmWifiConnectPollTimer=null;

function stopWifiConnectPolling(){
  if(_wmWifiConnectPollTimer){
    clearInterval(_wmWifiConnectPollTimer);
    _wmWifiConnectPollTimer=null;
  }
}

function pollWifiConnectStatus(){
  stopWifiConnectPolling();
  _wmWifiConnectPollTimer=setInterval(function(){
    api('/api/wifi/connect-status').then(function(res){
      var j={};
      var msg=$('wm-wifi-msg');
      try{j=JSON.parse(res.body);}catch(e){}
      if(msg&&j.message){
        msg.innerHTML=esc(j.message);
      }
      if(j.state==='success'){
        stopWifiConnectPolling();
        showToast(j.message||'WiFi connected',false);
        location.hash='#/';
      }else if(j.state==='failed'){
        stopWifiConnectPolling();
        showToast(j.message||'WiFi connect failed',true);
      }
    }).catch(function(){
      stopWifiConnectPolling();
      var msg=$('wm-wifi-msg');
      if(msg)msg.innerHTML='Portal became unreachable while checking connection status.';
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

// --- Info view ---
function viewInfo(){
  setView(navBar('info')+"<div class='wm-card'><p class='wm-lead'>Loading device info…</p></div>");
  api('/api/info').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar('info');
    html+="<div class='wm-page-head'><h1>Device info</h1><p class='wm-page-desc'>Connection status, hardware, and actions.</p></div>";
    function section(title,rows){
      if(!rows||!rows.length)return'';
      var h="<div class='wm-card'><h2 class='wm-card-title'>"+esc(title)+"</h2><dl class='wm-kv'>";
      for(var i=0;i<rows.length;i++){
        h+="<dt>"+esc(rows[i].label)+"</dt><dd>"+rows[i].value+"</dd>";
      }
      h+="</dl></div>";
      return h;
    }
    var st=d.status;
    if(st && typeof st==='object' && !Array.isArray(st)){
      html+="<div class='wm-card'><h2 class='wm-card-title'>Status</h2><dl class='wm-kv'>";
      html+="<dt>Connected</dt><dd>"+esc(String(st.connected))+"</dd>";
      if(st.ssid!==undefined)html+="<dt>SSID</dt><dd>"+esc(st.ssid)+"</dd>";
      if(st.stationIp)html+="<dt>Station IP</dt><dd>"+esc(st.stationIp)+"</dd>";
      if(st.apIp)html+="<dt>AP IP</dt><dd>"+esc(st.apIp)+"</dd>";
      if(st.summary)html+="<dt>Summary</dt><dd>"+esc(st.summary)+"</dd>";
      html+="</dl></div>";
    } else {
      html+=section('Status',d.status||[]);
    }
    html+=section('Device',d.device||[]);
    html+=section('WiFi',d.wifi||[]);
    html+=section('About',d.about||[]);
    if(d.extraSections){
      var xi,zs,zrows,zi,it;
      for(xi=0;xi<d.extraSections.length;xi++){
        zs=d.extraSections[xi];
        zrows=[];
        if(zs.items){for(zi=0;zi<zs.items.length;zi++){it=zs.items[zi];zrows.push({label:it.label,value:esc(it.value)});}}
        html+=section(zs.title,zrows);
      }
    }
    var act=d.actions||{};
    html+="<div class='wm-card'><h2 class='wm-card-title'>Actions</h2><div class='wm-btn-group'>";
    if(act.showRestart)html+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-restart-btn'>Restart device</button>";
    if(act.showExitPortal)html+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-exit-btn'>Exit portal</button>";
    if(act.showCloseCaptive)html+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-close-captive-btn'>Stop captive portal</button>";
    if(act.showUpdate)html+="<a class='wm-btn wm-btn--secondary wm-btn--block' href='#/update'>Firmware update</a>";
    if(act.showErase)html+="<button type='button' class='wm-btn wm-btn--danger wm-btn--block' id='wm-erase-btn'>Erase WiFi config</button>";
    html+="</div></div>";
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

// --- Params / OTA ---
function viewSetup(){
  setView(navBar('setup')+"<div class='wm-card'><p class='wm-lead'>Loading parameters…</p></div>");
  api('/api/params').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar('setup');
    html+="<div class='wm-page-head'><h1>Setup</h1><p class='wm-page-desc'>Custom parameters stored on this device.</p></div>";
    html+="<div class='wm-card'><form id='wm-param-form'>";
    html+=renderFieldList(d.params||[]);
    html+="<div class='wm-form-actions'><button type='submit' class='wm-btn wm-btn--primary wm-btn--block'>Save parameters</button></div>";
    html+="</form><div id='wm-param-msg'></div></div>";
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
  var pages=boot.pages||{};
  var pupd=pages.update||{};
  var html=navBar('update');
  html+="<div class='wm-page-head'><h1>Firmware update</h1><p class='wm-page-desc'>Flash a new firmware build (.bin). The device reboots when done.</p></div>";
  if(!pupd.visible){
    html+="<div class='wm-card'><p class='wm-lead'>Firmware update is disabled for this configuration.</p></div>";
    setView(html);
    return;
  }
  html+="<div class='wm-card'><form id='wm-ota-form'>";
  html+="<div class='wm-field'><label for='wm-ota-file'>Firmware file</label>";
  html+="<input type='file' id='wm-ota-file' name='update' accept='.bin,.bin.gz'/></div>";
  html+="<div class='wm-form-actions'><button type='submit' class='wm-btn wm-btn--primary wm-btn--block'>Upload firmware</button></div>";
  html+="</form>";
  html+="<p class='wm-lead' style='margin-top:14px;margin-bottom:8px'>Choose the correct binary for this chip. Upload progress appears below.</p>";
  html+="<pre id='wm-ota-log'></pre></div>";
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

// --- Routing ---
function route(){
  var h=location.hash||'#/';
  if(h.indexOf('#/')!==0)h='#/';
  var pages=boot.pages||{};
  var ps=pages.setup||{};
  var pi=pages.info||{};
  var pu=pages.update||{};
  if(h==='#/setup'&&ps.visible===false){viewHome();try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:'#/'}}));}catch(e){}return;}
  if(h==='#/info'&&pi.visible===false){viewHome();try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:'#/'}}));}catch(e){}return;}
  if(h==='#/update'&&pu.visible===false){viewHome();try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:'#/'}}));}catch(e){}return;}
  if(h==='#/'||h==='#'){viewHome();}
  else if(h==='#/wifi'){viewWifi();}
  else if(h==='#/info'){viewInfo();}
  else if(h==='#/setup'){viewSetup();}
  else if(h==='#/update'){viewUpdate();}
  else {viewHome();h='#/';}
  try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:h}}));}catch(e){}
}

function bootPortal(){
  route();
  try{document.dispatchEvent(new CustomEvent('wm:ready',{detail:{boot:boot}}));}catch(e){}
}

window.addEventListener('hashchange',route);
window.addEventListener('load',bootPortal);
})();

)rawliteral";

#endif  // _WM_PORTAL_APP_JS_H_
